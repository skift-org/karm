module;

#include <karm/macros>

export module Karm.Http:cache;

import Karm.Core;
import Karm.Ref;
import Karm.Debug;
import :transport;

namespace Karm::Http {

static auto debugNoCache = Debug::Flag::debug("http-nocache", "Disable http caching");

struct CacheTransport : Transport {
    Rc<Transport> _next;
    Map<Ref::Url, Rc<Ref::Blob>> _cached;

    CacheTransport(Rc<Transport> next) : _next(next) {}

    Rc<Response> _createResponse(Rc<Request> request, Rc<Ref::Blob> blob) {
        auto response = makeRc<Response>();
        response->version = request->version;
        response->code = OK;
        response->body = Body::from(blob);
        response->header.put(Header::CONTENT_TYPE, blob->type.str());

        return response;
    }

    Async::Task<Rc<Response>> doAsync(Rc<Request> request, Async::CancellationToken ct) override {
        if (request->method != GET)
            co_return co_await _next->doAsync(request, ct);

        if (not _cached.has(request->url)) {
            auto serverResponse = co_trya$(_next->doAsync(request, ct));

            if (serverResponse->code != OK)
                co_return Ok(serverResponse);

            if (not serverResponse->body)
                co_return Ok(serverResponse);

            auto contentType = serverResponse->header.contentType();
            auto data = co_trya$(Aio::readAllAsync(**serverResponse->body, ct));
            auto blob = makeRc<Ref::Blob>(contentType.unwrapOr("application/octet-stream"_mime), std::move(data));
            _cached.put(request->url, blob);

            auto response = _createResponse(request, blob);
            response->header.put("X-Karm-Cache"_sym, "miss"s);
            co_return Ok(response);
        }

        auto blob = _cached.get(request->url);
        auto response = _createResponse(request, blob);
        response->header.put("X-Karm-Cache"_sym, "hit"s);
        co_return Ok(response);
    }
};

export Rc<Transport> cacheTransport(Rc<Transport> next) {
    if (debugNoCache)
        return next;
    return makeRc<CacheTransport>(next);
}

export Rc<Transport> cacheTransport(Vec<Rc<Transport>> transports) {
    auto multiplex = makeRc<MultiplexTransport>(std::move(transports));
    return cacheTransport(multiplex);
}

} // namespace Karm::Http
