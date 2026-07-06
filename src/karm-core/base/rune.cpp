module;

#include <karm/macros>

export module Karm.Core:base.rune;

import :meta.id;
import :meta.traits;

import :base.array;
import :base.buf;
import :base.cursor;
import :base.endian;

namespace Karm {

export using Rune = u32;

export constexpr Rune REPLACEMENT = U'�';

export template <typename T>
concept StaticEncoding = requires(T t, Rune& r, typename T::Unit u, Cursor<typename T::Unit>& c, MutCursor<typename T::Unit>& m) {
    { T::unitLen(u) } -> Meta::Same<usize>;
    { T::runeLen(r) } -> Meta::Same<usize>;
    { T::decodeUnit(r, c) } -> Meta::Same<bool>;
    { T::encodeUnit(Rune{}, m) } -> Meta::Same<bool>;
};

export template <typename U, usize N>
struct _Multiple {
    using Inner = U;
    using Unit = U;

    InlineBuf<Unit, N> _buf{};

    always_inline void put(Unit u) {
        _buf.emplace(_buf.len(), u);
    }

    always_inline constexpr Unit& operator[](usize i) { return _buf[i]; }

    always_inline constexpr Unit const& operator[](usize i) const { return _buf[i]; }

    always_inline always_inline constexpr Unit* buf() { return _buf.buf(); }

    always_inline constexpr Unit const* buf() const { return _buf.buf(); }

    always_inline constexpr usize len() const { return _buf.len(); }

    always_inline constexpr usize rem() const { return N - len(); }
};

export template <typename U>
struct _Single {
    using Inner = U;
    using Unit = U;

    Unit _buf;

    always_inline _Single() = default;

    always_inline _Single(Unit u)
        : _buf(u) {}

    always_inline void put(Unit u) {
        _buf = u;
    }

    always_inline operator Unit() {
        return _buf;
    }

    always_inline constexpr Unit& operator[](usize) { return _buf; }

    always_inline constexpr Unit const& operator[](usize) const { return _buf; }

    always_inline constexpr Unit* buf() { return &_buf; }

    always_inline constexpr Unit const* buf() const { return &_buf; }

    always_inline constexpr usize len() const { return 1; }
};

export template <typename T, typename U>
concept EncodeOutput = requires(T t, U u) {
    { t.put(u) };
};

export template <typename T, typename U>
concept DecodeInput = requires(T t, U u) {
    { t.next() };
    { t.rem() };
};

// MARK: Utf8 ------------------------------------------------------------------

export struct Utf8 {
    using Unit = char;
    using One = _Multiple<Unit, 4>;
    static constexpr Array PREAMBLE = {0xef, 0xbb, 0xbf};

    always_inline static constexpr usize unitLen(Unit first) {
        if ((first & 0xf8) == 0xf0)
            return 4;
        else if ((first & 0xf0) == 0xe0)
            return 3;
        else if ((first & 0xe0) == 0xc0)
            return 2;
        else
            return 1;
    }

    always_inline static constexpr usize runeLen(Rune rune) {
        if (rune <= 0x7f)
            return 1;
        else if (rune <= 0x7ff)
            return 2;
        else if (rune <= 0xffff)
            return 3;
        else
            return 4;
    }

    always_inline static bool decodeUnit(Rune& result, DecodeInput<Unit> auto& in) {
        if (in.rem() == 0) {
            result = U'�';
            return false;
        }

        Unit first = in.next();

        if (unitLen(first) > in.rem() + 1) {
            result = U'�';
            return false;
        }

        if ((first & 0xf8) == 0xf0) {
            result = ((0x07 & first) << 18) |
                     ((0x3f & in.next()) << 12) |
                     ((0x3f & in.next()) << 6) |
                     ((0x3f & in.next()));
        } else if ((first & 0xf0) == 0xe0) {
            result = ((0x0f & first) << 12) |
                     ((0x3f & in.next()) << 6) |
                     ((0x3f & in.next()));
        } else if ((first & 0xe0) == 0xc0) {
            result = ((0x1f & first) << 6) |
                     ((0x3f & in.next()));
        } else {
            result = first;
        }

        return true;
    }

    always_inline static bool encodeUnit(Rune c, EncodeOutput<Unit> auto& out) {
        if (unitLen(c) > out.rem()) [[unlikely]]
            panic("bad");

        if (c <= 0x7f) {
            out.put(c);
        } else if (c <= 0x7ff) {
            out.put(0xc0 | (c >> 6));
            out.put(0x80 | (c & 0x3f));
        } else if (c <= 0xffff) {
            out.put(0xe0 | (c >> 12));
            out.put(0x80 | ((c >> 6) & 0x3f));
            out.put(0x80 | (c & 0x3f));
        } else if (c <= 0x10ffff) {
            out.put(0xf0 | (c >> 18));
            out.put(0x80 | ((c >> 12) & 0x3f));
            out.put(0x80 | ((c >> 6) & 0x3f));
            out.put(0x80 | (c & 0x3f));
        } else {
            return encodeUnit(U'�', out);
        }

        return true;
    }
};

export Utf8 UTF8;

static_assert(StaticEncoding<Utf8>);

// MARK: Utf16 -----------------------------------------------------------------

export template <typename T>
struct _Utf16 {
    using Unit = T;
    using One = _Multiple<Unit, 2>;
    static constexpr Array PREAMBLE = {Unit(0xef), 0xbb, 0xbf};

    always_inline static constexpr usize unitLen(Unit first) {
        if (first >= 0xd800 and first <= 0xdbff)
            return 2;
        else
            return 1;
    }

    always_inline static constexpr usize runeLen(Rune rune) {
        if (rune <= 0xffff)
            return 1;
        else
            return 2;
    }

    always_inline static bool decodeUnit(Rune& result, DecodeInput<Unit> auto& in) {
        if (in.rem() == 0) {
            result = U'�';
            return false;
        }

        u16 first = in.next();

        if (first >= 0xd800 and first <= 0xdbff) {
            if (in.rem() == 0) {
                result = U'�';
                return false;
            }

            u16 second = in.next();

            if (second < 0xdc00 or second > 0xdfff) {
                result = U'�';
                return false;
            }

            result = 0x10000 + ((first - 0xd800) << 10) + (second - 0xdc00);
        } else {
            result = first;
        }

        return true;
    }

    always_inline static bool encodeUnit(Rune c, EncodeOutput<Unit> auto& out) {
        if (c <= 0xffff) {
            out.put(c);
            return true;
        } else if (c <= 0x10ffff) {
            out.put(0xd800 | ((c - 0x10000) >> 10));
            out.put(0xdc00 | ((c - 0x10000) & 0x3ff));
            return true;
        } else {
            return encodeUnit(U'�', out);
        }
    }
};

export using Utf16 = _Utf16<u16>;
export Utf16 UTF16;
static_assert(StaticEncoding<Utf16>);

export using Utf16be = _Utf16<u16be>;
export Utf16be UTF16BE;
static_assert(StaticEncoding<Utf16be>);

export using Utf16le = _Utf16<u16le>;
export Utf16le UTF16Le;
static_assert(StaticEncoding<Utf16le>);

// MARK: Utf32 -----------------------------------------------------------------

export struct Utf32 {
    using Unit = char32_t;
    using One = _Single<Unit>;
    static constexpr Array PREAMBLE = {Unit(0xef), 0xbb, 0xbf};

    always_inline static constexpr usize unitLen(Unit) {
        return 1;
    }

    always_inline static constexpr usize runeLen(Rune) {
        return 1;
    }

    always_inline static bool decodeUnit(Rune& result, DecodeInput<Unit> auto& in) {
        result = in.next();
        return true;
    }

    always_inline static bool encodeUnit(Rune c, EncodeOutput<Unit> auto& out) {
        out.put(c);
        return true;
    }
};

export Utf32 UTF32;

static_assert(StaticEncoding<Utf32>);

// MARK: Ascii -----------------------------------------------------------------

export struct Ascii {
    using Unit = char;
    using One = _Single<Unit>;

    always_inline static constexpr usize unitLen(Unit) {
        return 1;
    }

    always_inline static constexpr usize runeLen(Rune) {
        return 1;
    }

    always_inline static bool decodeUnit(Rune& result, DecodeInput<Unit> auto& in) {
        auto c = in.next();
        if (c >= 0) {
            result = c;
            return true;
        } else {
            result = U'�';
            return false;
        }
    }

    always_inline static bool encodeUnit(Rune c, EncodeOutput<Unit> auto& out) {
        if (c < 0) {
            out.put('?');
            return false;
        }

        out.put(c);
        return true;
    }
};

export Ascii ASCII;

static_assert(StaticEncoding<Ascii>);

// MARK: Extended Ascii --------------------------------------------------------

export template <typename Mapper>
struct EAscii {
    using Unit = u8;
    using One = _Single<Unit>;

    always_inline static constexpr usize unitLen(Unit) {
        return 1;
    }

    always_inline static constexpr usize runeLen(Rune) {
        return 1;
    }

    always_inline static bool decodeUnit(Rune& result, DecodeInput<Unit> auto& in) {
        Mapper mapper;
        result = mapper(in.next());
        return true;
    }

    always_inline static bool encodeUnit(Rune c, EncodeOutput<Unit> auto& out) {
        Mapper mapper;
        for (usize i = 0; i <= 255; i++) {
            if (mapper(i) == c) {
                out.put(i);
                return true;
            }
        }
        out.put('?');
        return false;
    }
};

// MARK: Ibm437 ----------------------------------------------------------------

// clang-format off

struct Ibm437Mapper {
    Rune operator()(u8 c) const {
        Array mappings = {
            U'\0', U'☺', U'☻', U'♥', U'♦', U'♣', U'♠', U'•',
            U'◘', U'○', U'◙', U'♂', U'♀', U'♪', U'♫', U'☼',
            U'►', U'◄', U'↕', U'‼', U'¶', U'§', U'▬', U'↨',
            U'↑', U'↓', U'→', U'←', U'∟', U'↔', U'▲', U'▼',
            U' ', U'!', U'"', U'#', U'$', U'%', U'&', U'\'',
            U'(', U')', U'*', U'+', U',', U'-', U'.', U'/',
            U'0', U'1', U'2', U'3', U'4', U'5', U'6', U'7',
            U'8', U'9', U':', U';', U'<', U'=', U'>', U'?',
            U'@', U'A', U'B', U'C', U'D', U'E', U'F', U'G',
            U'H', U'I', U'J', U'K', U'L', U'M', U'N', U'O',
            U'P', U'Q', U'R', U'S', U'T', U'U', U'V', U'W',
            U'X', U'Y', U'Z', U'[', U'\\', U']', U'^', U'_',
            U'`', U'a', U'b', U'c', U'd', U'e', U'f', U'g',
            U'h', U'i', U'j', U'k', U'l', U'm', U'n', U'o',
            U'p', U'q', U'r', U's', U't', U'u', U'v', U'w',
            U'x', U'y', U'z', U'{', U'|', U'}', U'~',  U'⌂',
            U'Ç', U'ü', U'é', U'â', U'ä', U'à', U'å', U'ç',
            U'ê', U'ë', U'è', U'ï', U'î', U'ì', U'Ä', U'Å',
            U'É', U'æ', U'Æ', U'ô', U'ö', U'ò', U'û', U'ù',
            U'ÿ', U'Ö', U'Ü', U'¢', U'£', U'¥', U'₧', U'ƒ',
            U'á', U'í', U'ó', U'ú', U'ñ', U'Ñ', U'ª', U'º',
            U'¿', U'⌐', U'¬', U'½', U'¼', U'¡', U'«', U'»',
            U'░', U'▒', U'▓', U'│', U'┤', U'╡', U'╢', U'╖',
            U'╕', U'╣', U'║', U'╗', U'╝', U'╜', U'╛', U'┐',
            U'└', U'┴', U'┬', U'├', U'─', U'┼', U'╞', U'╟',
            U'╚', U'╔', U'╩', U'╦', U'╠', U'═', U'╬', U'╧',
            U'╨', U'╤', U'╥', U'╙', U'╘', U'╒', U'╓', U'╫',
            U'╪', U'┘', U'┌', U'█', U'▄', U'▌', U'▐', U'▀',
            U'α', U'ß', U'Γ', U'π', U'Σ', U'σ', U'µ', U'τ',
            U'Φ', U'Θ', U'Ω', U'δ', U'∞', U'φ', U'ε', U'∩',
            U'≡', U'±', U'≥', U'≤', U'⌠', U'⌡', U'÷', U'≈',
            U'°', U'∙', U'·', U'√', U'ⁿ', U'²', U'■', U'\x00a0',
        };

        return mappings[c];
    }
};

// clang-format on

export using Ibm437 = EAscii<Ibm437Mapper>;

export Ibm437 IBM437;

static_assert(StaticEncoding<Ibm437>);

// MARK: Latin1 ----------------------------------------------------------------

using Latin1Mapper = decltype([](u8 c) {
    // HACK: """"Unicode is a "superset" of Latin1""" (please note the quotes)
    return static_cast<Rune>(c);
});

export using Latin1 = EAscii<Latin1Mapper>;

export Latin1 LATIN1;

static_assert(StaticEncoding<Latin1>);

// MARK: Utilities -------------------------------------------------------------

export template <StaticEncoding Source, StaticEncoding Target>
usize transcodeLen(Cursor<typename Source::Unit> input) {
    usize result = 0;

    while (input.rem()) {
        Rune r;
        bool valid = Source::decodeUnit(r, input);
        result += Target::runeLen(valid ? r : U'�');
    }

    return result;
}

export template <StaticEncoding Source>
usize transcodeLen(Cursor<typename Source::Unit> input) {
    usize result = 0;

    while (input.rem()) {
        Rune r;
        if (Source::decodeUnit(r, input)) {
            result += 1;
        }
    }

    return result;
}

export template <StaticEncoding Source, StaticEncoding Target>
usize transcodeUnits(Cursor<typename Source::Unit> input, MutCursor<typename Target::Unit> output) {
    usize result = 0;

    while (input.rem()) {
        Rune r;
        bool valid = Source::decodeUnit(r, input);
        result += Target::encodeUnit(valid ? r : U'�', output);
    }

    return result;
}

export template <StaticEncoding T>
using One = typename T::One;

export template <StaticEncoding E>
bool encodeOne(Rune rune, One<E>& one) {
    return E::encodeUnit(rune, one);
}

export template <StaticEncoding E>
bool encodeOne(One<E>& one, Rune& rune) {
    return E::decodeUnit(rune, one);
}

export template <StaticEncoding E>
Slice<typename E::Unit> stripPreamble(Slice<typename E::Unit> units) {
    if constexpr (requires { E::PREAMBLE; })
        if (startWith(units, E::PREAMBLE) == Match::PARTIAL)
            return next(units, E::PREAMBLE.len());
    return units;
}

} // namespace Karm
