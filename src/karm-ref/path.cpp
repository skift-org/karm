module;

#include <karm-core/macros.h>

export module Karm.Ref:path;

// https://url.spec.whatwg.org/

import Karm.Core;

namespace Karm::Ref {

// MARK: Path ------------------------------------------------------------------

export Str suffixOf(Str str) {
    auto dotIndex = lastIndexOf(str, '.');
    if (not dotIndex.has())
        return "";
    return next(str, *dotIndex + 1);
}

export struct Path {
    static constexpr auto SEP = '/';

    bool rooted = false;
    Vec<String> _segs;

    static Path parse(Io::SScan& s, bool inUrl = false, bool stopAtWhitespace = false) {
        Path path;

        if (s.skip(SEP)) {
            path.rooted = true;
        }

        s.begin();
        while (not s.ended()) {
            if (inUrl and (s.peek() == '?' or s.peek() == '#'))
                break;

            if (stopAtWhitespace and isAsciiSpace(s.peek()))
                break;

            if (s.peek() == SEP) {
                path._segs.pushBack(s.end());
                s.next();
                s.begin();
            } else {
                s.next();
            }
        }

        auto last = s.end();
        if (last.len() > 0)
            path._segs.pushBack(last);

        return path;
    }

    static Path parse(Str str, bool inUrl = false, bool stopAtWhitespace = false) {
        Io::SScan s{str};
        return parse(s, inUrl, stopAtWhitespace);
    }

    void normalize() {
        Vec<String> parts;
        for (auto const& part : _segs) {
            if (part == ".")
                continue;

            if (part == "..") {
                if (parts.len() > 0) {
                    parts.popBack();
                } else if (not rooted) {
                    parts.pushBack(part);
                }
            } else
                parts.pushBack(part);
        }

        _segs = parts;
    }

    Str basename() const {
        if (not _segs.len())
            return {};

        return last(_segs);
    }

    Path join(Path const& other) const {
        if (other.rooted)
            return other;

        Path path = *this;
        path._segs.pushBack(other._segs);
        path.normalize();
        return path;
    }

    Path join(Str other) const {
        return join(parse(other));
    }

    void append(Str part) {
        _segs.pushBack(part);
    }

    Path parent(usize n = 1) const {
        Path path = *this;
        path._segs.resize(path._segs.len() > n ? path._segs.len() - n : 0);
        return path;
    }

    bool isParentOf(Path const& other) const {
        if (len() > other.len())
            return false;

        for (usize i = 0; i < len(); i++) {
            if (_segs[i] != other._segs[i])
                return false;
        }

        return true;
    }

    Res<> unparse(Io::TextWriter& writer) const {
        if (not rooted and len() == 0)
            try$(writer.writeRune('.'));

        if (rooted and len() == 0)
            try$(writer.writeRune(SEP));

        bool first = not rooted;

        for (auto const& part : _segs) {
            if (not first)
                try$(writer.writeRune(SEP));
            try$(writer.writeStr(part.str()));
            first = false;
        }

        return Ok();
    }

    String str() const {
        Io::StringWriter writer;
        unparse(writer).unwrap("unparse error");
        return writer.str();
    }

    Slice<String> parts() const {
        return _segs;
    }

    auto iter() const {
        return Karm::iter(_segs);
    }

    Str operator[](usize i) const {
        return _segs[i];
    }

    usize len() const {
        return _segs.len();
    }

    bool operator==(Path const&) const = default;

    auto operator<=>(Path const&) const = default;

    Str suffix() const {
        if (not _segs.len())
            return "";
        auto dotIndex = lastIndexOf(last(_segs), '.');
        if (not dotIndex.has())
            return "";
        return next(last(_segs), *dotIndex + 1);
    }
};

} // namespace Karm::Ref

export auto operator""_path(char const* str, Karm::usize len) {
    return Karm::Ref::Path::parse({str, len});
}

export auto operator/(Karm::Ref::Path const& path, Karm::Ref::Path const& other) {
    return path.join(other);
}

export auto operator/(Karm::Ref::Path const& path, Karm::Str other) {
    return path.join(other);
}

export template <>
struct Karm::Io::Formatter<Karm::Ref::Path> {
    Res<> format(Io::TextWriter& writer, Karm::Ref::Path const& path) {
        return path.unparse(writer);
    }
};
