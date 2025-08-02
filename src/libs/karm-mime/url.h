#pragma once

// https://url.spec.whatwg.org/
import Karm.Core;

#include "path.h"

namespace Karm::Mime {

struct Url {
    String scheme;
    String userInfo;
    String host;
    Opt<usize> port;
    Path path;
    String query;
    String fragment;

    static Url parse(Io::SScan& s, Opt<Url> baseUrl = NONE);

    static Url parse(Str str, Opt<Url> baseUrl = NONE);

    static bool isUrl(Str str);

    bool rooted() const {
        return path.rooted;
    }

    Url join(Path const& other) const;

    Url join(Str other) const;

    Str basename() const;

    void append(Str part) {
        path.append(part);
    }

    Url parent(usize n = 0) const;

    bool isParentOf(Url const& other) const;

    Res<> unparse(Io::TextWriter& writer) const;

    String str() const;

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

    static Res<Url> resolveReference(Url const& baseUrl, Url const& referenceUrl, bool strict = true);
};

Url parseUrlOrPath(Str str, Opt<Url> baseUrl = NONE);

} // namespace Karm::Mime

inline Karm::Mime::Url operator""_url(char const* str, Karm::usize len) {
    return Karm::Mime::Url::parse({str, len});
}

inline Karm::Mime::Url operator/(Karm::Mime::Url const& url, Karm::Str path) {
    return url.join(path);
}

inline Karm::Mime::Url operator/(Karm::Mime::Url const& url, Karm::Mime::Path const& path) {
    return url.join(path);
}

template <>
struct Karm::Io::Formatter<Karm::Mime::Url> {
    Karm::Res<> format(Karm::Io::TextWriter& writer, Karm::Mime::Url const& url) {
        return url.unparse(writer);
    }
};
