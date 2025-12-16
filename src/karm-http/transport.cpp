module;

#include <karm-core/macros.h>

export module Karm.Http:transport;

import Karm.Core;
import Karm.Ref;
import Karm.Sys;
import Karm.Logger;

import :request;
import :response;

namespace Karm::Http {

export struct Transport {
    virtual ~Transport() = default;

    virtual Async::Task<Rc<Response>> doAsync(Rc<Request> request, Async::CancellationToken ct) = 0;
};

// MARK: Http Transport --------------------------------------------------------

constexpr usize BUF_SIZE = 4096;

struct ContentBody : Body {
    usize _resumesPos = 0;
    Rc<Sys::TcpConnection> _conn;
    usize _contentLength;

    ContentBody(Rc<Sys::TcpConnection> conn, usize contentLength)
        : _conn(conn),
          _contentLength(contentLength) {
    }

    Async::Task<usize> readAsync(MutBytes buf, Async::CancellationToken ct) override {
        if (_contentLength == 0)
            co_return 0;

        usize n = min(buf.len(), _contentLength);
        n = co_trya$(_conn->readAsync(mutSub(buf, 0, n), ct));
        _contentLength -= n;
        co_return n;
    }
};

struct ChunkedBody : Body {
    Vec<u8> _buf;   // buffer of raw bytes (chunk headers + data)
    usize _pos = 0; // current read position in _buf
    Rc<Sys::TcpConnection> _conn;

    usize _chunkRemaining = 0; // bytes left to read in current chunk
    bool _eof = false;         // reached terminating 0-chunk

    ChunkedBody(Rc<Sys::TcpConnection> conn)
        : _conn(std::move(conn)) {
    }

    usize _available() const {
        return _buf.len() - _pos;
    }

    // Compact buffer when we've consumed a good part of it.
    void _compact() {
        if (_pos == 0)
            return;

        if (_pos == _buf.len()) {
            _buf = {};
            _pos = 0;
            return;
        }

        // Keep only [ _pos, end )
        _buf = Vec<u8>(sub(_buf, _pos, _buf.len()));
        _pos = 0;
    }

    Async::Task<> _fillAsync(Async::CancellationToken ct) {
        if (_eof)
            co_return Ok();

        _compact();

        Array<u8, BUF_SIZE> tmp = {};
        usize n = co_trya$(_conn->readAsync(tmp, ct));
        if (n == 0) {
            // connection closed unexpectedly
            _eof = true;
            co_return Ok();
        }

        for (usize i = 0; i < n; ++i)
            _buf.pushBack(tmp[i]);

        co_return Ok();
    }

    Async::Task<> _ensureAsync(usize need, Async::CancellationToken ct) {
        while (_available() < need and not _eof) {
            co_trya$(_fillAsync(ct));
            if (_available() == 0 and _eof)
                break;
        }
        co_return Ok();
    }

    // Read a single CRLF-terminated line into `line` (without CRLF).
    // Returns false on EOF before a full line.
    Async::Task<bool> _readLineAsync(Bytes& line, Async::CancellationToken ct) {
        while (true) {
            for (usize i = _pos; i + 1 < _buf.len(); ++i) {
                if (_buf[i] == '\r' and _buf[i + 1] == '\n') {
                    line = sub(_buf, _pos, i);
                    _pos = i + 2;
                    co_return true;
                }
            }

            if (_eof) {
                co_return false;
            }

            co_trya$(_fillAsync(ct));
        }
    }

    static usize _hexDigit(u8 c) {
        if (c >= '0' and c <= '9')
            return c - '0';
        if (c >= 'a' and c <= 'f')
            return 10 + (c - 'a');
        if (c >= 'A' and c <= 'F')
            return 10 + (c - 'A');
        return Limits<usize>::MAX;
    }

    static Res<usize> _parseChunkSize(Bytes line) {
        usize size = 0;
        for (usize i = 0; i < line.len(); ++i) {
            u8 c = line[i];

            if (c == ';')
                break;

            auto d = _hexDigit(c);
            if (d == Limits<usize>::MAX)
                continue;

            size = (size << 4) | d;
        }
        return Ok(size);
    }

    Async::Task<> _readNextChunkHeaderAsync(Async::CancellationToken ct) {
        Bytes line{};
        bool ok = co_trya$(_readLineAsync(line, ct));
        if (not ok) {
            _eof = true;
            co_return Ok();
        }

        auto size = co_try$(_parseChunkSize(line));
        if (size == 0) {
            while (true) {
                Bytes trailer{};
                bool has = co_trya$(_readLineAsync(trailer, ct));
                if (not has)
                    break;
                if (trailer.len() == 0)
                    break;
            }
            _eof = true;
            _chunkRemaining = 0;
            co_return Ok();
        }

        _chunkRemaining = size;
        co_return Ok();
    }

    Async::Task<usize> readAsync(MutBytes out, Async::CancellationToken ct) override {
        if (_eof)
            co_return 0;

        usize written = 0;

        while (written < out.len()) {
            if (_eof)
                break;

            // Need a new chunk?
            if (_chunkRemaining == 0) {
                co_trya$(_readNextChunkHeaderAsync(ct));
                if (_eof)
                    break;
            }

            if (_chunkRemaining == 0)
                continue;

            usize toCopy = min(_chunkRemaining, out.len() - written);
            co_trya$(_ensureAsync(toCopy, ct));
            usize avail = min(toCopy, _available());

            if (avail == 0) {
                // EOF mid-chunk
                _eof = true;
                break;
            }

            copy(sub(_buf, _pos, _pos + avail), mutSub(out, written, written + avail));

            _pos += avail;
            written += avail;
            _chunkRemaining -= avail;

            // Finished this chunk: must consume trailing CRLF.
            if (_chunkRemaining == 0) {
                co_trya$(_ensureAsync(2, ct));
                if (_available() >= 2 and
                    _buf[_pos] == '\r' and
                    _buf[_pos + 1] == '\n') {
                    _pos += 2;
                } else {
                    // invalid framing, bail out
                    _eof = true;
                    break;
                }
            }
        }

        if (written == 0 and _eof)
            co_return 0;

        co_return written;
    }
};

struct HttpTransport : Transport {
    Async::Task<> _sendRequestAsync(Request& request, Sys::TcpConnection& conn, Async::CancellationToken ct) {
        Io::StringWriter req;
        request.version = Version{1, 1};
        co_try$(request.unparse(req));
        co_trya$(conn.writeAsync(req.bytes(), ct));

        if (auto body = request.body)
            co_trya$(Aio::copyAsync(**body, conn, ct));

        co_return Ok();
    }

    Async::Task<Rc<Response>> _recvResponseAsync(Rc<Sys::TcpConnection> conn, Async::CancellationToken ct) {
        auto response = co_trya$(Response::readAsync(*conn, ct));

        if (auto contentLength = response.header.contentLength()) {
            response.body = makeRc<ContentBody>(conn, contentLength.unwrap());
        } else if (auto transferEncoding = response.header.tryGet(Header::TRANSFER_ENCODING)) {
            // For now we only support plain "chunked".
            if (*transferEncoding == "chunked") {
                response.body = makeRc<ChunkedBody>(conn);
            } else {
                logWarn("Transfer-Encoding: {} not supported", *transferEncoding);
            }
        } else {
            // NOTE: When there is no content length, and no transfer encoding,
            //       we read until the server closes the socket.
            response.body = makeRc<ContentBody>(conn, Limits<usize>::MAX);
        }

        co_return Ok(makeRc<Response>(std::move(response)));
    }

    Async::Task<Rc<Response>> doAsync(Rc<Request> request, Async::CancellationToken ct) override {
        auto& url = request->url;
        if (url.scheme != "http")
            co_return Error::unsupported("unsupported scheme");

        auto ips = co_trya$(Sys::lookupAsync(url.host.str()));
        auto port = url.port.unwrapOr(80);
        Sys::SocketAddr addr{first(ips), (u16)port};
        auto conn = makeRc<Sys::TcpConnection>(co_try$(Sys::TcpConnection::connect(addr)));
        co_trya$(_sendRequestAsync(*request, *conn, ct));
        co_return co_trya$(_recvResponseAsync(conn, ct));
    }
};

export Rc<Transport> httpTransport() {
    return makeRc<HttpTransport>();
}

// MARK: Pipe Transport --------------------------------------------------------

struct PipeBody : Body {
    usize _contentLength;

    PipeBody(usize contentLength = Limits<usize>::MAX)
        : _contentLength(contentLength) {
    }

    Async::Task<usize> readAsync(MutBytes buf, Async::CancellationToken) override {

        if (_contentLength == 0)
            co_return 0;

        usize n = min(buf.len(), _contentLength);
        n = co_try$(Sys::in().read(mutSub(buf, 0, n)));
        _contentLength -= n;

        co_return n;
    }
};

struct PipeTransport : Transport {
    Async::Task<> _sendRequest(Request& request, Async::CancellationToken ct) {
        request.version = Version{1, 1};
        co_try$(request.unparse(Sys::out()));

        if (auto body = request.body) {
            auto out = Aio::adapt(Sys::out());
            co_trya$(Aio::copyAsync(**body, out, ct));
        }

        co_return Ok();
    }

    Async::Task<Rc<Response>> _recvResponse(Async::CancellationToken ct) {
        auto response = co_trya$(Response::readAsync(Sys::in(), ct));
        if (auto contentLength = response.header.contentLength()) {
            response.body = makeRc<PipeBody>(contentLength.unwrap());
        } else {
            response.body = makeRc<PipeBody>();
        }

        co_return Ok(makeRc<Response>(std::move(response)));
    }

    Async::Task<Rc<Response>> doAsync(Rc<Request> request, Async::CancellationToken ct) override {
        auto& url = request->url;
        if (url.scheme != "pipe")
            co_return Error::unsupported("unsupported scheme");

        co_trya$(_sendRequest(*request, ct));
        co_return co_trya$(_recvResponse(ct));
    }
};

export Rc<Transport> pipeTransport() {
    return makeRc<PipeTransport>();
}

// MARK: Local -----------------------------------------------------------------

export enum struct LocalTransportPolicy {
    FILTER,
    ALLOW_ALL, // Allow all local resources access, this includes file:, bundle:, and fd:.
};

struct LocalTransport : Transport {
    LocalTransportPolicy _policy;
    Vec<String> _allowed = {};

    LocalTransport(LocalTransportPolicy policy)
        : _policy(policy) {}

    LocalTransport(Vec<String> allowed)
        : _policy(LocalTransportPolicy::FILTER), _allowed(allowed) {}

    Res<Pair<Rc<Body>, Ref::Mime>> _load(Ref::Url url) {
        if (url.scheme == "data") {
            auto blob = try$(url.blob);
            auto body = Body::from(blob);
            return Ok(Pair{body, blob->type});
        }

        if (try$(Sys::isFile(url))) {
            auto body = Body::from(try$(Sys::File::open(url)));
            auto mime = Ref::sniffSuffix(url.path.suffix()).unwrapOr("application/octet-stream"_mime);
            return Ok(Pair{body, mime});
        }

        auto dir = try$(Sys::Dir::open(url));
        Io::StringWriter sw;
        Io::Emit e{sw};
        e("<html><body><h1>Index of {}</h1><ul>", url.path);
        for (auto& diren : dir.entries()) {
            if (diren.hidden())
                continue;
            e("<li><a href=\"{}\">{}</a></li>", url.join(diren.name), diren.name);
        }
        e("</ul></body></html>");
        return Ok(Pair{Body::from(sw.take()), "text/html"_mime});
    }

    Async::Task<> _saveAsync(Ref::Url url, Rc<Body> body, Async::CancellationToken ct) {
        auto file = co_try$(Sys::File::create(url));
        co_trya$(Aio::copyAsync(*body, file, ct));
        co_return Ok();
    }

    Async::Task<Rc<Response>> doAsync(Rc<Request> request, Async::CancellationToken ct) override {
        if (_policy == LocalTransportPolicy::FILTER) {
            if (not contains(_allowed, request->url.scheme)) {
                co_return Error::permissionDenied("disallowed by policy");
            }
        }

        auto response = makeRc<Response>();
        response->code = OK;

        if (auto it = request->body;
            it and (request->method == PUT or
                    request->method == POST))
            co_trya$(_saveAsync(request->url, *it, ct));

        if (request->method == GET or request->method == POST) {
            auto [body, mime] = co_try$(_load(request->url));
            response->body = body;
            response->header.put(Header::CONTENT_TYPE, mime.str());
        }

        co_return Ok(response);
    }
};

export Rc<Transport> localTransport(LocalTransportPolicy policy) {
    return makeRc<LocalTransport>(policy);
}

export Rc<Transport> localTransport(Vec<String> allowed) {
    return makeRc<LocalTransport>(std::move(allowed));
}

// MARK: Fallback --------------------------------------------------------------

struct MultiplexTransport : Transport {
    Vec<Rc<Transport>> _transports;

    MultiplexTransport(Vec<Rc<Transport>> transports)
        : _transports(std::move(transports)) {}

    Async::Task<Rc<Response>> doAsync(Rc<Request> request, Async::CancellationToken ct) override {
        for (auto& transport : _transports) {
            auto res = co_await transport->doAsync(request, ct);
            if (res)
                co_return res.unwrap();

            if (res.none() != Error::UNSUPPORTED)
                co_return res.none();
        }

        co_return Error::unsupported("no client could handle the request");
    }
};

export Rc<Transport> multiplexTransport(Vec<Rc<Transport>> transports) {
    return makeRc<MultiplexTransport>(std::move(transports));
}

} // namespace Karm::Http
