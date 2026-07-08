module;

#include <karm/macros>

export module Karm.Auth.Jwt;

import Karm.Core;
import Karm.Crypto;

using namespace Karm::Literals;

namespace Karm::Auth {

struct Claims : Map<String, String> {
    static constexpr Str TYPE = "typ";
    static constexpr Str CONTENT_TYPE = "cty";

    static constexpr Str ISSUER = "iss";
    static constexpr Str SUBJECT = "sub";
    static constexpr Str AUDIENCE = "aud";
    static constexpr Str EXPIRATION = "exp";
    static constexpr Str NOT_BEFORE = "nbf";
    static constexpr Str ISSUED_AT = "iat";
    static constexpr Str JWT_ID = "jti";

    static Res<Claims> parse(Io::SScan& s) {
        Claims m;
        if (not s.skip('{'))
            return Error::invalidData("expected '{'");

        if (s.skip('}'))
            return Ok(m);

        while (true) {
            s.eat(Re::space());
            auto key = try$(Json::parseStr(s));

            s.eat(Re::space());
            if (not s.skip(':'))
                return Error::invalidData("expected ':'");

            s.eat(Re::space());

            auto value = try$(Json::parseStr(s));
            m.put(key, value);

            s.eat(Re::space());
            if (s.skip('}'))
                return Ok(m);

            if (not s.skip(','))
                return Error::invalidData("expected ','");
        }
    }

    void unparse(Io::Emit& e) const {
        e('{');
        bool first = true;
        for (auto const& [k, v] : iterItems()) {
            if (not first) {
                e(',');
            }
            first = false;

            e('"');
            Json::escape(e, k);
            e("\":");
            Json::escape(e, k);
        }
        e('}');
    }
};

export struct Jwt {
    Claims header;
    Claims payload;
    Opt<String> signature;

    static Res<Tuple<Str, Str, Str>> split(Str token) {
        auto iter = iterSplit(token, '.');
        Str header = try$(iter.next().okOr(Error::invalidData("jwt is missing header")));
        Str payload = try$(iter.next().okOr(Error::invalidData("jwt is missing payload")));
        Str signature = try$(iter.next().okOr(Error::invalidData("jwt is missing signature")));
        return Ok(Tuple{header, payload, signature});
    }
};

} // namespace Karm::Auth
