module;

#include <karm/macros>

export module Karm.Http:client;

import Karm.Core;
import Karm.Debug;
import Karm.Ref;
import Karm.Logger;

import :transport;

namespace Karm::Http {

static auto debugClient = Debug::Flag::debug("http-client"s, "Log HTTP client requests"s);
static auto debugClientExtra = Debug::Flag::debug("http-client-extra"s, "Log extra info about HTTP client requests"s);

export struct Client : Transport {
    String userAgent = "Karm-Http/" stringify$(__ck_version_value) ""s;
    Rc<Transport> _transport;

    Client(Rc<Transport> transport)
        : _transport(std::move(transport)) {}

    Async::Task<Rc<Response>> doAsync(Rc<Request> request, Async::CancellationToken ct) override {
        request->header.put(Header::USER_AGENT, userAgent);
        auto maybeResp = co_await _transport->doAsync(request, ct);
        if (not maybeResp) {
            logErrorIf(debugClient, "\"{} {}\" {}", request->method, request->url, maybeResp.none());
            co_return maybeResp.none();
        }

        auto resp = maybeResp.unwrap();
        if (debugClient or debugClientExtra) {
            Io::StringWriter sw;
            if (debugClientExtra) {
                if (auto cache = resp->header.tryGet("X-Karm-Cache"_sym))
                    co_try$(Io::format(sw, " ({})"s, cache));
            }
            logInfo("\"{} {}\" {} {}{}", request->method, request->url, toUnderlyingType(resp->code), resp->code, sw.take());
        }
        co_return Ok(resp);
    }

    [[clang::coro_wrapper]]
    Async::Task<Rc<Response>> getAsync(Ref::Url url, Async::CancellationToken ct) {
        auto req = makeRc<Request>();
        req->method = Method::GET;
        req->url = url;
        req->header.put(Header::HOST, url.host.str());

        return doAsync(req, ct);
    }

    [[clang::coro_wrapper]]
    Async::Task<Rc<Response>> headAsync(Ref::Url url, Async::CancellationToken ct) {
        auto req = makeRc<Request>();
        req->method = Method::HEAD;
        req->url = url;
        req->header.put(Header::HOST, url.host.str());

        return doAsync(req, ct);
    }

    [[clang::coro_wrapper]]
    Async::Task<Rc<Response>> postAsync(Ref::Url url, Rc<Body> body, Async::CancellationToken ct) {
        auto req = makeRc<Request>();
        req->method = Method::POST;
        req->url = url;
        req->body = body;
        req->header.put(Header::HOST, url.host.str());

        return doAsync(req, ct);
    }

    [[clang::coro_wrapper]]
    Async::Task<Rc<Response>> putAsync(Ref::Url url, Rc<Body> body, Async::CancellationToken ct) {
        auto req = makeRc<Request>();
        req->method = Method::PUT;
        req->url = url;
        req->body = body;
        req->header.put(Header::HOST, url.host.str());

        return doAsync(req, ct);
    }

    [[clang::coro_wrapper]]
    Async::Task<Rc<Response>> deleteAsync(Ref::Url url, Async::CancellationToken ct) {
        auto req = makeRc<Request>();
        req->method = Method::DELETE;
        req->url = url;
        req->header.put(Header::HOST, url.host.str());

        return doAsync(req, ct);
    }
};

// MARK: Clientless ------------------------------------------------------------

export Rc<Client> defaultClient() {
    return makeRc<Client>(
        multiplexTransport({
            httpTransport(),
            localTransport(LocalTransportPolicy::ALLOW_ALL),
        })
    );
}

export Async::Task<Rc<Response>> getAsync(Ref::Url url, Async::CancellationToken ct) {
    auto client = defaultClient();
    co_return co_await client->getAsync(url, ct);
}

export Async::Task<Rc<Response>> headAsync(Ref::Url url, Async::CancellationToken ct) {
    auto client = defaultClient();
    co_return co_await client->headAsync(url, ct);
}

export Async::Task<Rc<Response>> postAsync(Ref::Url url, Rc<Body> body, Async::CancellationToken ct) {
    auto client = defaultClient();
    co_return co_await client->postAsync(url, body, ct);
}

export Async::Task<Rc<Response>> putAsync(Ref::Url url, Rc<Body> body, Async::CancellationToken ct) {
    auto client = defaultClient();
    co_return co_await client->putAsync(url, body, ct);
}

export Async::Task<Rc<Response>> deleteAsync(Ref::Url url, Async::CancellationToken ct) {
    auto client = defaultClient();
    co_return co_await client->deleteAsync(url, ct);
}

export Async::Task<Rc<Response>> doAsync(Rc<Request> request, Async::CancellationToken ct) {
    auto client = defaultClient();
    co_return co_await client->doAsync(request, ct);
}

} // namespace Karm::Http
