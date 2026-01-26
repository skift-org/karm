module;

#include <karm/macros>

export module Karm.Http:server;

import Karm.Core;
import Karm.Logger;

import :request;
import :response;
import :transport;

namespace Karm::Http {

export struct Handler {
    virtual ~Handler() = default;
    virtual Async::Task<> handleAsync(Rc<Request>, Rc<ResponseWriter>, Async::CancellationToken ct) = 0;
};

template <typename F>
concept HandlerFunc = requires(F f, Rc<Request> req, Rc<ResponseWriter> resp, Async::CancellationToken ct) {
    { f(req, resp, ct) } -> Meta::Same<Async::Task<>>;
};

template <HandlerFunc F>
Rc<Handler> makeHandler(F func) {
    struct FuncHandler : Handler {
        F _func;

        FuncHandler(F func) : _func(std::move(func)) {}

        [[clang::coro_wrapper]]
        Async::Task<> handleAsync(Rc<Request> req, Rc<ResponseWriter> resp, Async::CancellationToken ct) override {
            return _func(req, resp, ct);
        }
    };

    return makeRc<FuncHandler>(func);
}

export struct ServerProps {
    String name = "Karm HTTP Server"s;
    Sys::SocketAddr addr = Sys::Ip4::localhost(80);
};

export struct Server {
    struct _ResponseWriter : Http::ResponseWriter {
        Rc<Sys::TcpConnection> _conn;
        bool _headerSent = false;

        _ResponseWriter(Rc<Sys::TcpConnection> conn)
            : _conn(conn) {}

        Async::Task<> writeHeaderAsync(Code code, Async::CancellationToken ct) override {
            Response resp;
            resp.version = Version{1, 1};
            resp.code = code;
            resp.header = header;
            // NOTE: Seems to be the average size of an HTTP header
            Io::StringWriter sb{1024};
            co_try$(resp.unparse(sb));
            co_trya$(_conn->writeAsync(sb.bytes(), ct));
            _headerSent = true;
            co_return Ok();
        }

        Async::Task<usize> writeAsync(Bytes buf, Async::CancellationToken ct) override {
            if (not _headerSent) {
                if (not header.has(Header::CONTENT_TYPE))
                    header.put(Header::CONTENT_TYPE, Ref::sniffBytes(buf).str());
                if (not header.has(Header::CONTENT_LENGTH))
                    header.put(Header::CONTENT_LENGTH, Io::format("{}", buf.len()));
                co_trya$(writeHeaderAsync(code, ct));
            }
            co_return co_await _conn->writeAsync(buf, ct);
        }
    };

    static Rc<Server> simple(Rc<Handler> handler, ServerProps const& props) {
        return makeRc<Server>(handler, props);
    }

    Rc<Handler> _handler;
    ServerProps _props;

    Async::Task<Rc<Request>> _recvRequestAsync(Rc<Sys::TcpConnection> conn, Async::CancellationToken ct) {
        auto request = co_trya$(Request::readAsync(*conn, ct));
        if (auto contentLength = request.header.contentLength()) {
            request.body = makeRc<ContentBody>(conn, contentLength.unwrap());
        } else if (auto transferEncoding = request.header.tryGet(Header::TRANSFER_ENCODING)) {
            logWarn("Transfer-Encoding: {} not supported", transferEncoding);
        } else {
            // NOTE: When there is no content length, and no transfer encoding,
            //       we read until the client closes the socket.
            request.body = makeRc<ContentBody>(conn, Limits<usize>::MAX);
        }

        co_return Ok(makeRc<Request>(std::move(request)));
    }

    Async::Task<> _handleConnectionAsync(Rc<Sys::TcpConnection> conn, Async::CancellationToken ct) {
        bool keepAlive = true;
        while (keepAlive) {
            auto req = co_trya$(_recvRequestAsync(conn, ct));
            logInfo("{} {} {}", req->method, req->url, req->version);
            if (auto connection = req->header.access(Header::CONNECTION)) {
                if (eqCi(connection->str(), "close"s)) {
                    keepAlive = false;
                } else if (eqCi(connection->str(), "keep-alive"s)) {
                    keepAlive = true;
                }
            }

            auto resp = makeRc<_ResponseWriter>(conn);
            resp->header.put(Header::SERVER, _props.name);
            if (not keepAlive) {
                resp->header.put(Header::CONNECTION, "close"s);
            } else {
                resp->header.put(Header::CONNECTION, "keep-alive"s);
            }

            co_trya$(_handler->handleAsync(req, resp, ct));
        }
        co_return Ok();
    }

    Async::Task<> serveAsync(Async::CancellationToken ct) {
        auto listener = co_try$(Sys::TcpListener::listen(_props.addr));
        logInfo("{#} listening on http://{}", _props.name, _props.addr);
        while (true) {
            auto connection = makeRc<Sys::TcpConnection>(co_trya$(listener.acceptAsync(ct)));
            Async::detach(_handleConnectionAsync(connection, ct));
        }
    }
};

// MARK: Serverless ------------------------------------------------------------

export Async::Task<> serveAsync(Rc<Handler> handler, ServerProps props, Async::CancellationToken ct) {
    auto server = Server::simple(handler, props);
    co_return co_await server->serveAsync(ct);
}

export [[clang::coro_wrapper]]
Async::Task<> serveAsync(Rc<Handler> handler, Async::CancellationToken ct) {
    return serveAsync(handler, {}, ct);
}

export [[clang::coro_wrapper]]
Async::Task<> serveAsync(HandlerFunc auto handler, ServerProps const& props, Async::CancellationToken ct) {
    return serveAsync(makeHandler(handler), props, ct);
}

export [[clang::coro_wrapper]]
Async::Task<> serveAsync(HandlerFunc auto handler, Async::CancellationToken ct) {
    return serveAsync(makeHandler(handler), {}, ct);
}

} // namespace Karm::Http
