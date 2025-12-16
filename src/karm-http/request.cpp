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

    static Async::Task<Request> readAsync(Aio::Reader& r, Async::CancellationToken ct) {
        auto headers = co_trya$(readHeadersAsync(r, ct));
        Io::SScan scan{bytes(headers).cast<char>()};
        co_return parse(scan);
    }

    static Res<Request> read(Io::Reader& r) {
        auto headers = try$(readHeaders(r));
        Io::SScan scan{bytes(headers).cast<char>()};
        return parse(scan);
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

    Async::Task<Serde::Value> readJsonAsync(Async::CancellationToken ct) {
        if (not body)
            co_return Error::invalidInput("request has no body");
        co_return co_await body.unwrap()->readJsonAsync(ct);
    }
};

} // namespace Karm::Http
