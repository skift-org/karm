module;

#include <karm-core/macros.h>

export module Karm.Http:header;

import Karm.Core;
import Karm.Ref;

namespace Karm::Http {

// MARK: Version ---------------------------------------------------------------

export struct Version {
    u8 major;
    u8 minor;

    static Res<Version> parse(Io::SScan& s) {
        if (not s.skip("HTTP/"))
            return Error::invalidData("Expected \"HTTP/\"");
        Version v;
        v.major = try$(atou(s));
        s.skip('.');
        v.minor = try$(atou(s));
        return Ok(v);
    }

    Res<> unparse(Io::TextWriter& w) const {
        try$(Io::format(w, "HTTP/{}.{}", major, minor));
        return Ok();
    }

    bool operator==(Version const& other) const = default;

    auto operator<=>(Version const& other) const = default;
};

// MARK: Header ----------------------------------------------------------------

export struct Header : Map<Symbol, String> {
    using Map::Map;

    static Symbol const CONTENT_LENGTH;
    static Symbol const CONTENT_TYPE;
    static Symbol const CONTENT_ENCODING;
    static Symbol const CONTENT_LANGUAGE;
    static Symbol const CONTENT_DISPOSITION;
    static Symbol const ACCEPT;
    static Symbol const ACCEPT_ENCODING;
    static Symbol const ACCEPT_LANGUAGE;
    static Symbol const HOST;
    static Symbol const USER_AGENT;
    static Symbol const REFERER;
    static Symbol const ORIGIN;
    static Symbol const CACHE_CONTROL;
    static Symbol const PRAGMA;
    static Symbol const EXPIRES;
    static Symbol const AUTHORIZATION;
    static Symbol const WWW_AUTHENTICATE;
    static Symbol const COOKIE;
    static Symbol const SET_COOKIE;
    static Symbol const CONNECTION;
    static Symbol const UPGRADE;
    static Symbol const TRANSFER_ENCODING;
    static Symbol const TE;
    static Symbol const RANGE;
    static Symbol const CONTENT_RANGE;
    static Symbol const LOCATION;
    static Symbol const ETAG;
    static Symbol const IF_NONE_MATCH;
    static Symbol const IF_MODIFIED_SINCE;
    static Symbol const DATE;
    static Symbol const SERVER;
    static Symbol const VIA;

    Res<> parse(Io::SScan& s) {
        while (not s.ended()) {
            Str key;
            Str value;

            auto RE_ENDLINE =
                Re::zeroOrMore(' '_re) & "\r\n"_re;

            auto RE_SEPARATOR =
                Re::separator(':'_re);

            auto RE_KEY_VALUE =
                Re::token(
                    key,
                    Re::until(RE_SEPARATOR)
                ) &
                RE_SEPARATOR &
                Re::token(
                    value,
                    Re::until(RE_ENDLINE)
                ) &
                RE_ENDLINE;

            if (s.skip("\r\n"))
                break;

            if (not s.skip(RE_KEY_VALUE))
                return Error::invalidData("Expected header");

            put(Symbol::from(key), value);
        }

        return Ok();
    }

    Res<> unparse(Io::TextWriter& w) const {
        for (auto& [key, value] : iterUnordered()) {
            try$(Io::format(w, "{}: {}\r\n", key, value));
        }
        return Ok();
    }

    Opt<usize> contentLength() {
        if (auto value = tryGet(CONTENT_LENGTH))
            return Io::atou(value->str());
        return NONE;
    }

    Opt<Ref::Mime> contentType() {
        if (auto value = tryGet(CONTENT_TYPE))
            return Ref::Mime{value->str()};
        return NONE;
    }
};

Symbol const Header::CONTENT_LENGTH = "Content-Length"_sym;
Symbol const Header::CONTENT_TYPE = "Content-Type"_sym;
Symbol const Header::CONTENT_ENCODING = "Content-Encoding"_sym;
Symbol const Header::CONTENT_LANGUAGE = "Content-Language"_sym;
Symbol const Header::CONTENT_DISPOSITION = "Content-Disposition"_sym;
Symbol const Header::ACCEPT = "Accept"_sym;
Symbol const Header::ACCEPT_ENCODING = "Accept-Encoding"_sym;
Symbol const Header::ACCEPT_LANGUAGE = "Accept-Language"_sym;
Symbol const Header::HOST = "Host"_sym;
Symbol const Header::USER_AGENT = "User-Agent"_sym;
Symbol const Header::REFERER = "Referer"_sym;
Symbol const Header::ORIGIN = "Origin"_sym;
Symbol const Header::CACHE_CONTROL = "Cache-Control"_sym;
Symbol const Header::PRAGMA = "Pragma"_sym;
Symbol const Header::EXPIRES = "Expires"_sym;
Symbol const Header::AUTHORIZATION = "Authorization"_sym;
Symbol const Header::WWW_AUTHENTICATE = "WWW-Authenticate"_sym;
Symbol const Header::COOKIE = "Cookie"_sym;
Symbol const Header::SET_COOKIE = "Set-Cookie"_sym;
Symbol const Header::CONNECTION = "Connection"_sym;
Symbol const Header::UPGRADE = "Upgrade"_sym;
Symbol const Header::TRANSFER_ENCODING = "Transfer-Encoding"_sym;
Symbol const Header::TE = "TE"_sym;
Symbol const Header::RANGE = "Range"_sym;
Symbol const Header::CONTENT_RANGE = "Content-Range"_sym;
Symbol const Header::LOCATION = "Location"_sym;
Symbol const Header::ETAG = "ETag"_sym;
Symbol const Header::IF_NONE_MATCH = "If-None-Match"_sym;
Symbol const Header::IF_MODIFIED_SINCE = "If-Modified-Since"_sym;
Symbol const Header::DATE = "Date"_sym;
Symbol const Header::SERVER = "Server"_sym;
Symbol const Header::VIA = "Via"_sym;

// MARK: Headers ---------------------------------------------------------------

export Async::Task<Vec<u8>> readHeadersAsync(Aio::Reader& r) {
    Io::BufferWriter bw;
    while (true) {
        auto adaptedBw = Aio::adapt(bw);
        auto [read, reachedDelim] = co_trya$(Aio::readLineAsync(r, adaptedBw, "\r\n"_bytes));

        if (not reachedDelim)
            co_return Error::invalidInput("input stream ended with incomplete http header");

        if (read == 0)
            break;
    }

    co_return Ok(bw.take());
}

export Res<Vec<u8>> readHeaders(Io::Reader& r) {
    Io::BufferWriter bw;
    while (true) {
        auto [read, reachedDelim] = try$(Io::readLine(r, bw, "\r\n"_bytes));

        if (not reachedDelim)
            return Error::invalidInput("input stream ended with incomplete http header");

        if (read == 0)
            break;
    }

    return Ok(bw.take());
}

} // namespace Karm::Http

template <>
struct Karm::Io::Formatter<Karm::Http::Version> {
    Res<> format(TextWriter& writer, Http::Version version) {
        return Io::format(writer, "HTTP/{}.{}", version.major, version.minor);
    }
};
