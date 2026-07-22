module;

#include <karm/macros>

export module Karm.Http:response;

import Karm.Core;
import Karm.Ref;
import :body;
import :code;
import :header;

namespace Karm::Http {

export struct ResponseWriter : Aio::Writer {
    Code code = OK;
    Header header;

    virtual Async::Task<> writeHeaderAsync(Code code, Async::CancellationToken ct) = 0;

    Async::Task<> writeJsonAsync(Serde::Value const& value, Async::CancellationToken ct) {
        auto string = co_try$(Json::unparse(value));
        co_trya$(writeAsync(bytes(string), ct));
        co_return Ok();
    }

    Async::Task<> writeStrAsync(Str str, Async::CancellationToken ct) {
        co_trya$(writeAsync(bytes(str), ct));
        co_return Ok();
    }

    Async::Task<> writeFileAsync(Ref::Url const& url, Async::CancellationToken ct) {
        auto f = co_try$(Sys::File::open(url));
        header.put(Header::CONTENT_LENGTH, Io::format("{}", co_try$(f.stat()).size));

        if (not header.contains(Header::CONTENT_TYPE)) {
            Array<u8, Ref::SNIFF_BUFFER_SIZE> buf;
            auto len = co_trya$(f.readAsync(buf, ct));
            Ref::Uti uti =
                (url.path.suffix()
                     ? Ref::Uti::fromSuffix(url.path.suffix())
                     : Ref::sniffBytes(sub(buf, 0, len)));
            if (uti.mimeTypes())
                header.put(Header::CONTENT_TYPE, uti.mimeTypes()[0].str());
            co_trya$(writeAsync(sub(buf, 0, len), ct));
        }

        co_trya$(Aio::copyAsync(f, *this, ct));
        co_return Ok();
    }

    Async::Task<> redirectAsync(Code code, Ref::Url location, Async::CancellationToken ct) {
        header.put(Header::LOCATION, location.str());
        co_return co_await writeHeaderAsync(code, ct);
    }

    Async::Task<> notFoundAsync(Async::CancellationToken ct) {
        code = NOT_FOUND;
        co_return co_await writeStrAsync("Not Found"s, ct);
    }
};

export struct Response {
    Version version;
    Code code = OK;
    Header header;
    Opt<Rc<Body>> body;

    static Res<Response> parse(Io::SScan& s) {
        Response res;

        res.version = try$(Version::parse(s));

        if (not s.skip(' '))
            return Error::invalidData("Expected space");

        res.code = try$(parseCode(s));

        if (not s.skip(' '))
            return Error::invalidData("Expected space");

        s.skip(Re::untilAndConsume("\r\n"_re));

        try$(res.header.parse(s));

        return Ok(res);
    }

    static Async::Task<Response> readAsync(Aio::Reader& r, Async::CancellationToken ct) {
        auto headers = co_trya$(readHeadersAsync(r, ct));
        Io::SScan scan{bytes(headers).cast<char>()};
        co_return parse(scan);
    }

    static Res<Response> read(Io::Reader& r) {
        auto headers = try$(readHeaders(r));
        Io::SScan scan{bytes(headers).cast<char>()};
        return parse(scan);
    }

    Res<> unparse(Io::TextWriter& w) const {
        // Start line
        try$(Io::format(w, "{} {} {}", version, toUnderlyingType(code), toStr(code)));
        try$(w.writeStr("\r\n"s));

        // Headers and empty line
        try$(header.unparse(w));
        try$(w.writeStr("\r\n"s));

        return Ok();
    }
};

} // namespace Karm::Http
