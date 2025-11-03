module;

#include <karm-core/macros.h>

export module Karm.Http:server;

import Karm.Core;
import Karm.Logger;

import :request;
import :response;
import :transport;

namespace Karm::Http {

using Params = Map<String, String>;

using Handler = SharedFunc<Async::Task<>(Rc<Request>, Rc<Response::Writer>)>;

export struct Service {
    virtual ~Service() = default;
    virtual Async::Task<> handleAsync(Rc<Request>, Rc<Response::Writer>) = 0;
};

export struct Server {
    static Rc<Server> simple(Sys::SocketAddr addr, Rc<Service> srv) {
        return makeRc<Server>(addr, srv);
    }

    Sys::SocketAddr _addr;
    Rc<Service> _srv;

    Async::Task<Rc<Request>> _recvRequestAsync(Sys::TcpConnection& conn) {
        Array<u8, BUF_SIZE> buf = {};
        Io::BufReader reader = sub(
            buf, 0, co_trya$(conn.readAsync(buf))
        );

        auto request = co_try$(Request::read(reader));
        if (auto contentLength = request.header.contentLength()) {
            request.body = makeRc<ContentBody>(reader.bytes(), std::move(conn), contentLength.unwrap());
        } else if (auto transferEncoding = request.header.tryGet("Transfer-Encoding"s)) {
            logWarn("Transfer-Encoding: {} not supported", transferEncoding);
        } else {
            // NOTE: When there is no content length, and no transfer encoding,
            //       we read until the client closes the socket.
            request.body = makeRc<ContentBody>(reader.bytes(), std::move(conn), Limits<usize>::MAX);
        }

        co_return Ok(makeRc<Request>(std::move(request)));
    }

    Async::Task<> _handleConnectionAsync(Sys::TcpConnection conn) {
        auto req = co_trya$(_recvRequestAsync(conn));

        struct ResponseWriter : Response::Writer {
            Sys::TcpConnection& _conn;
            Io::BufferWriter _bw;
            bool _headerSent = false;

            ResponseWriter(Sys::TcpConnection& conn)
                : _conn(conn) {}

            Async::Task<> writeHeaderAsync(Code code) override {
                Response resp;
                resp.version = Version{1, 1};
                resp.code = code;
                resp.header = header;
                _headerSent = true;

                co_return Ok();
            }

            Async::Task<usize> writeAsync(Bytes buf) override {
                if (not _headerSent)
                    co_trya$(writeHeaderAsync(Code::OK));

                auto written = co_try$(_bw.write(buf));

                co_return Ok(written);
            }
        };

        co_trya$(_srv->handleAsync(req, makeRc<ResponseWriter>(conn)));
        co_trya$(conn.flushAsync());
        co_return Ok();
    }

    Async::Task<> serveAsync() {
        auto listener = co_try$(Sys::TcpListener::listen(_addr));
        logInfo("Server listening on {}", _addr);
        while (true) {
            auto connection = co_trya$(listener.acceptAsync());
            Async::detach(_handleConnectionAsync(std::move(connection)));
        }
    }
};

// MARK: Serverless ------------------------------------------------------------

export Async::Task<> servAsync(Rc<Service> srv, Sys::SocketAddr addr = Sys::Ip4::localhost(80)) {
    auto server = Server::simple(addr, srv);
    co_return co_await server->serveAsync();
}

} // namespace Karm::Http
