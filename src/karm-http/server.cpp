module;

#include <karm-core/macros.h>

export module Karm.Http:server;

import Karm.Core;
import Karm.Logger;

import :request;
import :response;
import :transport;

namespace Karm::Http {

export struct Handler {
    virtual ~Handler() = default;
    virtual Async::Task<> handleAsync(Rc<Request>, Rc<Response::Writer>) = 0;
};

template <typename F>
concept HandlerFunc = requires(F f, Rc<Request> req, Rc<Response::Writer> resp) {
    { f(req, resp) } -> Meta::Same<Async::Task<>>;
};

template <HandlerFunc F>
Rc<Handler> makeHandler(F func) {
    struct FuncHandler : Handler {
        F _func;

        FuncHandler(F func) : _func(std::move(func)) {}

        [[clang::coro_wrapper]]
        Async::Task<> handleAsync(Rc<Request> req, Rc<Response::Writer> resp) override {
            return _func(req, resp);
        }
    };

    return makeRc<FuncHandler>(func);
}

export struct ServerProps {
    String name = "Karm HTTP Server"s;
    Sys::SocketAddr addr = Sys::Ip4::localhost(80);
};

export struct Server {
    static Rc<Server> simple(Rc<Handler> handler, ServerProps const& props) {
        return makeRc<Server>(handler, props);
    }

    Rc<Handler> _handler;
    ServerProps _props;

    Async::Task<Rc<Request>> _recvRequestAsync(Rc<Sys::TcpConnection> conn) {
        Array<u8, BUF_SIZE> buf = {};
        Io::BufReader reader = sub(
            buf, 0, co_trya$(conn->readAsync(buf))
        );

        auto request = co_try$(Request::read(reader));
        if (auto contentLength = request.header.contentLength()) {
            request.body = makeRc<ContentBody>(reader.bytes(), conn, contentLength.unwrap());
        } else if (auto transferEncoding = request.header.tryGet(Header::TRANSFER_ENCODING)) {
            logWarn("Transfer-Encoding: {} not supported", transferEncoding);
        } else {
            // NOTE: When there is no content length, and no transfer encoding,
            //       we read until the client closes the socket.
            request.body = makeRc<ContentBody>(reader.bytes(), conn, Limits<usize>::MAX);
        }

        co_return Ok(makeRc<Request>(std::move(request)));
    }

    Async::Task<> _handleConnectionAsync(Rc<Sys::TcpConnection> conn) {
        struct ResponseWriter : Response::Writer {
            Rc<Sys::TcpConnection> _conn;
            bool _headerSent = false;

            ResponseWriter(Rc<Sys::TcpConnection> conn)
                : _conn(conn) {}

            Async::Task<> writeHeaderAsync(Code code) override {
                Response resp;
                resp.version = Version{1, 1};
                resp.code = code;
                resp.header = header;

                Io::StringWriter sb;
                co_try$(resp.unparse(sb));
                co_trya$(_conn->writeAsync(sb.bytes()));
                _headerSent = true;
                co_return Ok();
            }

            Async::Task<usize> writeAsync(Bytes buf) override {
                if (not _headerSent) {
                    if (not header.has(Header::CONTENT_TYPE))
                        header.put(Header::CONTENT_TYPE, Ref::sniffBytes(buf).str());
                    co_trya$(writeHeaderAsync(Code::OK));
                }
                co_return co_await _conn->writeAsync(buf);
            }
        };

        auto req = co_trya$(_recvRequestAsync(conn));
        logInfo("{} {} {}", req->method, req->url, req->version);
        auto resp = makeRc<ResponseWriter>(std::move(conn));
        resp->header.put(Header::SERVER, _props.name);
        co_trya$(_handler->handleAsync(req, resp));
        co_return Ok();
    }

    Async::Task<> serveAsync() {
        auto listener = co_try$(Sys::TcpListener::listen(_props.addr));
        logInfo("{#} listening on http://{}", _props.name, _props.addr);
        while (true) {
            auto connection = makeRc<Sys::TcpConnection>(co_trya$(listener.acceptAsync()));
            Async::detach(_handleConnectionAsync(connection));
        }
    }
};

// MARK: Serverless ------------------------------------------------------------

export Async::Task<> serveAsync(Rc<Handler> handler, ServerProps const& props = {}) {
    auto server = Server::simple(handler, props);
    co_return co_await server->serveAsync();
}

export [[clang::coro_wrapper]]
Async::Task<> serveAsync(HandlerFunc auto handler, ServerProps const& props = {}) {
    return serveAsync(makeHandler(handler, props));
}

} // namespace Karm::Http
