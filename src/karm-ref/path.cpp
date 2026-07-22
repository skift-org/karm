module;

#include <karm/macros>

export module Karm.Ref:path;

// https://url.spec.whatwg.org/

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Ref {

// https://url.spec.whatwg.org/#concept-url-path
export struct Path {
    static constexpr auto SEPARATOR = '/';

    Vec<String> _segments;
    bool _absolute = false;

    static Path parse(Io::SScan& s, bool inUrl = false, bool stopAtWhitespace = false) {
        Path path;

        if (s.skip(SEPARATOR))
            path._absolute = true;

        s.begin();
        while (not s.ended()) {
            if (inUrl and (s.peek() == '?' or s.peek() == '#'))
                break;

            if (stopAtWhitespace and isAsciiSpace(s.peek()))
                break;

            if (s.peek() == SEPARATOR) {
                path._segments.pushBack(s.end());
                s.next();
                s.begin();
            } else {
                s.next();
            }
        }

        auto last = s.end();
        if (last.len() > 0 or path._segments.len() > 0)
            path._segments.pushBack(last);

        return path;
    }

    static Path parse(Str str, bool inUrl = false, bool stopAtWhitespace = false) {
        Io::SScan s{str};
        return parse(s, inUrl, stopAtWhitespace);
    }

    bool trailingSlash() const {
        if (_segments.len() <= 0)
            return false;

        auto const& lastSeg = last(_segments);
        return lastSeg == "" or lastSeg == "." or lastSeg == "..";
    }

    void stripTrailingSlash() {
        if (trailingSlash())
            _segments.popBack();
    }

    void normalize() {
        Vec<String> parts;

        for (auto const& part : _segments) {
            if (part == ".")
                continue;

            if (part == "..") {
                if (parts.len() > 0) {
                    if (last(parts) == "") {
                        parts.popBack();
                        if (parts.len() > 0)
                            parts.popBack();
                    } else {
                        parts.popBack();
                    }
                } else if (not _absolute) {
                    parts.pushBack(part);
                }
            } else {
                if (part == "" and _absolute and parts.len() == 0)
                    continue;
                parts.pushBack(part);
            }
        }

        if (trailingSlash()) {
            if (parts.len() == 0 or last(parts) != "") {
                if (parts.len() > 0 or not _absolute) {
                    parts.pushBack(""s);
                }
            }
        }

        _segments = parts;
    }

    void absolutize() {
        _absolute = true;
    }

    void relativize() {
        _absolute = false;
    }

    bool absolute() const {
        return _absolute;
    }

    void absolute(bool value) {
        _absolute = value;
    }

    bool relative() const {
        return not _absolute;
    }

    void relative(bool value) {
        _absolute = not value;
    }

    Str basename() const {
        if (not _segments.len())
            return {};

        return last(_segments);
    }

    Str stem() const {
        auto base = basename();
        if (not base)
            return "";
        auto dotIndex = lastIndexOf(base, '.');
        if (not dotIndex)
            return base;
        return sub(base, 0, *dotIndex);
    }

    Str suffix() const {
        auto base = basename();
        if (not base)
            return "";
        auto dotIndex = lastIndexOf(base, '.');
        if (not dotIndex)
            return "";
        return next(base, *dotIndex + 1);
    }

    Path join(Path const& other) const {
        if (other._absolute)
            return other;

        Path path = *this;
        path.stripTrailingSlash();
        path._segments.pushBack(other._segments);
        path.normalize();
        return path;
    }

    Path join(Str other) const {
        return join(parse(other));
    }

    void append(Str part) {
        if (_segments.len() > 0 and last(_segments) == "")
            _segments.popBack();
        stripTrailingSlash();
        _segments.pushBack(part);
    }

    Path parent(usize n = 1) const {
        Path path = *this;
        path._segments.resize(path._segments.len() > n ? path._segments.len() - n : 0);
        return path;
    }

    bool parentOf(Path const& other) const {
        if (len() > other.len())
            return false;

        for (usize i = 0; i < len(); i++) {
            if (_segments[i] != other._segments[i])
                return false;
        }

        return true;
    }

    Res<> unparse(Io::TextWriter& writer) const {
        if (not _absolute and len() == 0)
            try$(writer.writeRune('.'));

        if (_absolute and len() == 0)
            try$(writer.writeRune(SEPARATOR));

        bool first = not _absolute;

        for (auto const& part : _segments) {
            if (not first)
                try$(writer.writeRune(SEPARATOR));
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

    Slice<String> segments() const {
        if (trailingSlash())
            return sub(_segments, 0, _segments.len() - 1);
        return _segments;
    }

    usize len() const {
        return segments().len();
    }

    Slice<String> segmentsIncludingEndSlash() const {
        return _segments;
    }

    usize lenIncludingEndSlash() const {
        return _segments.len();
    }

    auto iter() const {
        return Karm::iter(segments());
    }

    Str operator[](usize i) const {
        return segments()[i];
    }

    void hash(Meta::Derive<Hasher> auto& h) const {
        Karm::hash(h, _absolute);
        Karm::hash(h, _segments);
    }

    bool operator==(Path const&) const = default;

    auto operator<=>(Path const&) const = default;
};

} // namespace Karm::Ref

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

namespace Karm::Ref::Literals {

export auto operator""_path(char const* str, Karm::usize len) {
    return Karm::Ref::Path::parse({str, len});
}

} // namespace Karm::Ref::Literals
