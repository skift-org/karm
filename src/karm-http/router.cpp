module;

#include <karm-core/macros.h>

export module Karm.Http:router;

import :server;
import :response;
import :request;

namespace Karm::Http {

export struct RoutePattern {
    struct Segment {
        enum struct Type {
            PARAM,
            PATH,
            EXTRA,
            _LEN,
        };

        using enum Type;

        Type type;
        String value;

        void repr(Io::Emit& e) const {
            e("({} {#})", type, value);
        }
    };

    Method _method;
    String _host;
    Vec<Segment> _segments;

    static RoutePattern parse(Io::SScan& s) {
        auto method = parseMethod(s).unwrap("invalid pattern method");
        return parse(method, s);
    }

    static RoutePattern parse(Str str) {
        Io::SScan scan{str};
        return parse(scan);
    }

    static RoutePattern parse(Method method, Io::SScan& s) {
        RoutePattern p;
        p._method = method;
        s.skip(Re::space());
        if (s.peek() != '/')
            p._host = s.token(Re::until('/'_re));

        bool seenExtra = false;

        while (not s.ended() and s.skip('/')) {
            Segment seg;
            if (seenExtra)
                panic("EXTRA segment must be the last segment");
            if (s.skip('{')) {
                seg.type = Segment::PARAM;
                seg.value = s.token(Re::until('}'_re | "..."_re));
                if (s.skip("...")) {
                    seg.type = Segment::EXTRA;
                    if (seenExtra)
                        panic("multiple EXTRA segments not allowed");
                    seenExtra = true;
                }
                s.skip('}');
            } else if (s.rem() == 0) {
                // ignore trailing slash
                break;
            } else {
                seg.type = Segment::PATH;
                seg.value = s.token(Re::until('/'_re));
            }
            p._segments.pushBack(seg);
        }

        return p;
    }

    static RoutePattern parse(Method method, Str str) {
        Io::SScan scan{str};
        return parse(method, scan);
    }

    Opt<RouteParams> match(Method method, Ref::Url const& url) {
        if (_method != method)
            return NONE;

        if (not Glob::matchGlob(_host, url.host.str()))
            return NONE;

        RouteParams params;

        Cursor parts = url.path.parts();

        for (auto& seg : _segments) {
            if (parts.ended())
                return NONE;

            switch (seg.type) {
            case Segment::PARAM:
                params.put(seg.value, parts.next());
                break;

            case Segment::PATH:
                if (seg.value != *parts)
                    return NONE;
                parts.next();
                break;

            case Segment::EXTRA: {
                StringBuilder sb;

                bool first = true;
                while (not parts.ended()) {
                    if (not first)
                        sb.append('/');
                    sb.append(parts.next());
                    first = false;
                }

                params.put(seg.value, sb.take());
                return params; // we're done, EXTRA is terminal
            }

            default:
                unreachable();
            }
        }

        if (not parts.ended())
            return NONE;

        return params;
    }

    void repr(Io::Emit& e) const {
        e("(pattern {} {} {})", _method, _host, _segments);
    }
};

export struct Router : Handler {
    Vec<Tuple<RoutePattern, Rc<Handler>>> _routes;

    void route(RoutePattern pattern, Rc<Handler> handler) {
        _routes.emplaceBack(pattern, handler);
    }

    void route(RoutePattern pattern, HandlerFunc auto handlerFunc) {
        return route(pattern, makeHandler(std::move(handlerFunc)));
    }

    void route(Str pattern, Rc<Handler> handler) {
        route(RoutePattern::parse(pattern), handler);
    }

    void route(Str pattern, HandlerFunc auto handlerFunc) {
        route(RoutePattern::parse(pattern), makeHandler(std::move(handlerFunc)));
    }

    void get(Str pattern, Rc<Handler> handler) {
        route(RoutePattern::parse(GET, pattern), handler);
    }

    void get(Str pattern, HandlerFunc auto handlerFunc) {
        route(RoutePattern::parse(GET, pattern), makeHandler(std::move(handlerFunc)));
    }

    void post(Str pattern, Rc<Handler> handler) {
        route(RoutePattern::parse(POST, pattern), handler);
    }

    void post(Str pattern, HandlerFunc auto handlerFunc) {
        route(RoutePattern::parse(POST, pattern), makeHandler(std::move(handlerFunc)));
    }

    void put(Str pattern, Rc<Handler> handler) {
        route(RoutePattern::parse(PUT, pattern), handler);
    }

    void put(Str pattern, HandlerFunc auto handlerFunc) {
        route(RoutePattern::parse(PUT, pattern), makeHandler(std::move(handlerFunc)));
    }

    void delete_(Str pattern, Rc<Handler> handler) {
        route(RoutePattern::parse(DELETE, pattern), handler);
    }

    void delete_(Str pattern, HandlerFunc auto handlerFunc) {
        route(RoutePattern::parse(DELETE, pattern), makeHandler(std::move(handlerFunc)));
    }

    Async::Task<> _handle404Async(Rc<Response::Writer> resp) {
        co_trya$(resp->writeHeaderAsync(Code::NOT_FOUND));
        co_return co_await resp->writeStrAsync("404 Not Found"s);
    }

    [[clang::coro_wrapper]]
    Async::Task<> handleAsync(Rc<Request> req, Rc<Response::Writer> resp) override {
        for (auto& [pattern, handler] : _routes) {
            logDebug("trying pattern {}: {}", pattern, req->url);
            if (auto params = pattern.match(req->method, req->url)) {
                req->routeParams = params.take();
                return handler->handleAsync(req, resp);
            }
        }

        return _handle404Async(resp);
    }
};

} // namespace Karm::Http
