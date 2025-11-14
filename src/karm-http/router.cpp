module;

#include <karm-core/macros.h>

export module Karm.Http:router;

import :server;

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
        RoutePattern p;
        p._method = parseMethod(s).unwrap("invalid pattern method");
        s.skip(Re::space());
        if (s.peek() != '/')
            p._host = s.token(Re::until('/'_re));

        bool seenExtra = false;

        while (not s.ended() and s.skip('/')) {
            Segment seg;
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
            } else {
                seg.type = Segment::PATH;
                seg.value = s.token(Re::until('/'_re));
            }
        }

        return p;
    }

    static RoutePattern parse(Str str) {
        Io::SScan scan{str};
        return parse(scan);
    }

    Opt<Params> match(Method method, Ref::Url const& url) {
        if (_method != method)
            return NONE;

        if (not Glob::matchGlob(_host, url.host.str()))
            return NONE;

        Params params;

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

using RouteHandlerAsync = SharedFunc<Async::Task<>(Rc<Request>, Rc<Response::Writer>)>;

export struct Router : Service {
    Vec<Tuple<RoutePattern, RouteHandlerAsync>> _routes;

    void route(RoutePattern pattern, RouteHandlerAsync handlerAsync) {
        _routes.emplaceBack(pattern, handlerAsync);
    }

    void route(Str pattern, RouteHandlerAsync handlerAsync) {
        route(RoutePattern::parse(pattern), handlerAsync);
    }

    Async::Task<> _handle404Async(Rc<Response::Writer> resp) {
        co_trya$(resp->writeHeaderAsync(Code::NOT_FOUND));
        co_return co_await resp->writeStrAsync("404 Not Found"s);
    }

    Async::Task<> handleAsync(Rc<Request> req, Rc<Response::Writer> resp) override {
        for (auto& [pattern, handler] : _routes) {
            if (auto params = pattern.match(req->method, req->url)) {
                co_return co_await handler(req, resp);
            }
        }

        co_return co_await _handle404Async(resp);
    }
};

} // namespace Karm::Http
