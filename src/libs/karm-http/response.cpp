module;

#include <karm-core/macros.h>

export module Karm.Http:response;

import Karm.Core;
import :body;
import :code;
import :header;

namespace Karm::Http {

export struct Response {
    Version version;
    Code code = OK;
    Header header;
    Opt<Rc<Body>> body;

    struct Writer : Aio::Writer {
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
    };

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
};

} // namespace Karm::Http
