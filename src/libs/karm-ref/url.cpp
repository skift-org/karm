module;

#include <karm-core/macros.h>

export module Karm.Ref:url;

// https://url.spec.whatwg.org/
import Karm.Core;

import :mime;
import :path;

namespace Karm::Ref {

auto const RE_COMPONENT =
    Re::alpha() &
    Re::zeroOrMore(
        Re::alnum() | '+'_re | '-'_re | '.'_re
    );

auto const RE_SCHEME = RE_COMPONENT & ':'_re;

export struct Url {
    String scheme;
    String userInfo;
    String host;
    Opt<usize> port;
    Path path;
    String query;
    String fragment;

    static Url parse(Io::SScan& s, Opt<Url> baseUrl = NONE) {
        Url url;

        if (s.ahead(RE_SCHEME)) {
            url.scheme = s.token(RE_COMPONENT);
            s.skip(':');
        }

        if (s.skip("//")) {
            auto maybeHost = s.token(RE_COMPONENT);

            if (s.skip('@')) {
                url.userInfo = maybeHost;
                maybeHost = s.token(RE_COMPONENT);
            }

            url.host = maybeHost;

            if (s.skip(':')) {
                url.port = atou(s);
            }
        }

        url.path = Path::parse(s, true);

        if (s.skip('?'))
            url.query = s.token(Re::until('#'_re));

        if (s.skip('#'))
            url.fragment = s.token(Re::until(Re::eof()));

        if (not baseUrl)
            return url;

        auto resolvedRef = resolveReference(baseUrl.unwrap(), url);
        return resolvedRef.unwrapOr(url);
    }


    static Url parse(Str str, Opt<Url> baseUrl = NONE) {
        Io::SScan s{str};
        return parse(s, baseUrl);
    }

    static bool isUrl(Str str)  {
        Io::SScan s{str};

        return s.skip(RE_COMPONENT) and
            s.skip(':');
    }


    bool rooted() const {
        return path.rooted;
    }

    Url join(Path const& other) const {
        Url url = *this;
        url.path = url.path.join(other);
        return url;
    }

    Url join(Str other) const {
        return join(Path::parse(other));
    }

    Str basename() const  {
        return path.basename();
    }

    void append(Str part) {
        path.append(part);
    }

    Url parent(usize n = 0) const {
        Url url = *this;
        url.path = url.path.parent(n);
        return url;
    }

    bool isParentOf(Url const& other) const {
        bool same = scheme == other.scheme and
                    host == other.host and
                    port == other.port;

        return same and path.isParentOf(other.path);
    }


    Res<> unparse(Io::TextWriter& writer) const  {
        if (scheme.len() > 0)
            try$(Io::format(writer, "{}:", scheme));

        if (userInfo.len() > 0 or host.len() > 0)
            try$(writer.writeStr("//"s));

        if (userInfo.len() > 0)
            try$(Io::format(writer, "{}@", userInfo));

        if (host.len() > 0)
            try$(writer.writeStr(host.str()));

        if (port)
            try$(Io::format(writer, ":{}", port.unwrap()));

        try$(path.unparse(writer));

        if (query.len() > 0)
            try$(Io::format(writer, "?{}", query));

        if (fragment.len() > 0)
            try$(Io::format(writer, "#{}", fragment));

        return Ok();
    }

    String str() const {
        Io::StringWriter writer;
        unparse(writer).unwrap("unparse error");
        return writer.str();
    }

    auto iter() const {
        return path.iter();
    }

    auto len() const {
        return path.len();
    }

    auto operator<=>(Url const&) const = default;

    bool isRelative() const {
        return not scheme;
    }

    static Path _mergePaths(Url const& baseUrl, Url const& referenceUrl) {
        if (baseUrl.host and baseUrl.path.len() == 0) {
            Path path = referenceUrl.path;
            path.rooted = true;
            return path;
        }

        return baseUrl.path.parent(1).join(referenceUrl.path);
    }

    // https://datatracker.ietf.org/doc/html/rfc3986#section-5.2.2
    static Res<Url> resolveReference(Url const& baseUrl, Url const& referenceUrl, bool strict = true) {
        if (baseUrl.isRelative())
            return Error::invalidInput("base url must not be a relative url");

        bool undefineReferenceSchema = not strict and referenceUrl.scheme == baseUrl.scheme;
        if (referenceUrl.scheme and not undefineReferenceSchema) {
            Url targetUrl = referenceUrl;
            targetUrl.path.normalize();
            return Ok(targetUrl);
        }

        if (referenceUrl.host) {
            Url targetUrl = referenceUrl;
            targetUrl.scheme = baseUrl.scheme;
            targetUrl.path.normalize();
            return Ok(targetUrl);
        }

        Url targetUrl = baseUrl;
        if (referenceUrl.path.len() == 0) {
            if (referenceUrl.query)
                targetUrl.query = referenceUrl.query;
        } else {
            if (referenceUrl.path.rooted)
                targetUrl.path = referenceUrl.path;
            else
                targetUrl.path = _mergePaths(baseUrl, referenceUrl);
            targetUrl.query = referenceUrl.query;
        }

        targetUrl.fragment = referenceUrl.fragment;
        targetUrl.path.normalize();

        return Ok(targetUrl);
    }
};

export Url parseUrlOrPath(Str str, Opt<Url> baseUrl = NONE)  {
    if (Url::isUrl(str)) {
        return Url::parse(str, baseUrl);
    }

    Url url = baseUrl.unwrapOr(Url::parse(""));
    url.path = url.path.join(Path::parse(str));

    return url;
}

} // namespace Karm::Ref

export Karm::Ref::Url operator""_url(char const* str, Karm::usize len) {
    return Karm::Ref::Url::parse({str, len});
}

export Karm::Ref::Url operator/(Karm::Ref::Url const& url, Karm::Str path) {
    return url.join(path);
}

export Karm::Ref::Url operator/(Karm::Ref::Url const& url, Karm::Ref::Path const& path) {
    return url.join(path);
}

export template <>
struct Karm::Io::Formatter<Karm::Ref::Url> {
    Karm::Res<> format(Karm::Io::TextWriter& writer, Karm::Ref::Url const& url) {
        return url.unparse(writer);
    }
};
