module;

#include <karm-core/macros.h>

export module Karm.Http:response;

import Karm.Core;
import :body;
import :code;
import :header;

namespace Karm::Http {

struct ResponseWriter : Aio::Writer {
    Code code = OK;
    Header header;

    virtual Async::Task<> writeHeaderAsync(Code code) = 0;

    Async::Task<> writeJsonAsync(Serde::Value const& value) {
        auto string = co_try$(Json::unparse(value));
        co_trya$(writeAsync(bytes(string)));
        co_return Ok();
    }

    Async::Task<> writeStrAsync(Str str) {
        co_trya$(writeAsync(bytes(str)));
        co_return Ok();
    }

    Async::Task<> writeFileAsync(Ref::Url const& url) {
        auto data = co_try$(Sys::readAllUtf8(url));
        co_return co_await writeStrAsync(data);
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

    static Async::Task<Response> readAsync(Aio::Reader& r) {
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

    static Res<Response> read(Io::Reader& r) {
        Io::BufferWriter bw;
        while (true) {
            auto [read, reachedDelim] = try$(Io::readLine(r, bw, "\r\n"_bytes));

            if (not reachedDelim)
                return Error::invalidInput("input stream ended with incomplete http header");

            if (read == 0)
                break;
        }

        Io::SScan scan{bw.bytes().cast<char>()};
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
