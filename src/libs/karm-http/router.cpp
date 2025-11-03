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

        while (not s.ended() and s.skip('/')) {
            Segment seg;
            if (s.skip('{')) {
                seg.type = Segment::PARAM;
                seg.value = s.token(Re::until('}'_re | "..."_re));
                if (s.skip("..."))
                    seg.type = Segment::EXTRA;
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

        return params;
    }

    void repr(Io::Emit& e) const {
        e("(pattern {} {} {})", _method, _host, _segments);
    }
};

export struct Router : Service {
    Vec<Tuple<RoutePattern, Handler>> _routes;

    void route(RoutePattern pattern, Handler func) {
        _routes.emplaceBack(pattern, func);
    }

    void route(Str pattern, Handler func) {
        route(RoutePattern::parse(pattern), func);
    }

    Async::Task<> handleAsync(Rc<Request>, Rc<Response::Writer>) override {
        co_return Error::notImplemented();
    }
};

} // namespace Karm::Http
