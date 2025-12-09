module;

#include <karm-core/macros.h>

export module Karm.Http:request;

import Karm.Ref;

import :body;
import :header;
import :method;

namespace Karm::Http {

export using RouteParams = Map<String, String>;

export struct Request {
    Method method;
    Ref::Url url;
    Version version;
    Header header;
    Opt<Rc<Body>> body;

    // Used by the router to pass path params
    RouteParams routeParams = {};

    static Rc<Request> from(Method method, Ref::Url url, Opt<Rc<Body>> body = NONE) {
        auto req = makeRc<Request>();

        req->method = method;
        req->url = url;
        req->header.put(Header::HOST, url.host.str());
        req->body = body;

        return req;
    }

    static Res<Request> parse(Io::SScan& s) {
        Request req;

        req.method = try$(parseMethod(s));

        if (not s.skip(' '))
            return Error::invalidData("Expected space");

        auto path = Ref::Path::parse(s, true, true);
        path.rooted = true;
        path.normalize();
        req.url.path = path;

        if (not s.skip(' '))
            return Error::invalidData("Expected space");

        req.version = try$(Version::parse(s));

        if (not s.skip("\r\n"))
            return Error::invalidData("Expected \"\\r\\n\"");

        try$(req.header.parse(s));

        return Ok(req);
    }

    static Async::Task<Request> readAsync(Aio::Reader& r) {
        Io::BufferWriter bw;
        while (true) {
            auto adaptedBw = Aio::adapt(bw);
            auto [read, reachedDelim] = co_trya$(Aio::readLineAsync(r, adaptedBw, "\r\n"_bytes));

            if (not reachedDelim)
                co_return Error::invalidInput("input stream ended with incomplete http header");

            if (read == 0)
                break;
        }

        Io::SScan scan{bw.bytes().cast<char>()};
        co_return parse(scan);
    }

    Res<> unparse(Io::TextWriter& w) const {
        // Start line
        auto path = url.path;
        path.rooted = true;
        try$(Io::format(w, "{} {} ", toStr(method), url.path));

        try$(version.unparse(w));
        try$(w.writeStr("\r\n"s));

        // Headers and empty line
        try$(header.unparse(w));
        try$(w.writeStr("\r\n"s));

        return Ok();
    }

    Async::Task<Serde::Value> readJsonAsync() {
        if (not body)
            co_return Error::invalidInput("request has no body");
        co_return co_await body.unwrap()->readJsonAsync();
    }
};

} // namespace Karm::Http
