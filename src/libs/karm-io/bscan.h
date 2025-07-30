#pragma once

#include <karm-base/cursor.h>
#include <karm-base/endian.h>
#include <karm-base/string.h>
#include <karm-base/vec.h>
#include <karm-io/traits.h>

#include "aton.h"

namespace Karm::Io {

// Inspired by https://github.com/citizenfx/fivem/blob/master/code/client/shared/Hooking.Patterns.h#L41
struct BPattern {
    Vec<u8> _pattern;
    Vec<u8> _mask;

    static void _fromSimple(Str pattern, Vec<u8>& out) {
        u8 tmp = 0;
        bool has = false;
        for (auto c : pattern) {
            if (c == ' ') {
                // ignore
            } else if (auto b = _parseDigit(c, {.base = 16})) {
                tmp <<= 4;
                tmp |= *b;
                if (has) {
                    out.pushBack(tmp);
                    tmp = 0;
                    has = false;
                } else {
                    has = true;
                }
            }
        }
    }

    static BPattern from(Str pattern, Str mask) {
        BPattern res;
        _fromSimple(pattern, res._pattern);
        _fromSimple(mask, res._mask);

        return res;
    }

    static BPattern from(Str pattern) {
        BPattern res;

        u8 tmp = 0;
        bool has = false;
        for (auto c : pattern) {
            if (c == ' ') {
                // ignore
            } else if (c == '?') {
                res._pattern.pushBack(0x00);
                res._mask.pushBack(0x00);
            } else if (auto b = _parseDigit(c, {.base = 16})) {
                tmp <<= 4;
                tmp |= *b;
                if (has) {
                    res._pattern.pushBack(tmp);
                    res._mask.pushBack(0xff);
                    tmp = 0;
                    has = false;
                } else {
                    has = true;
                }
            }
        }

        return res;
    }

    BPattern() = default;

    BPattern(Bytes pattern, Bytes mask) {
        if (pattern.len() != mask.len())
            panic("pattern len should be equal to mask len");

        _pattern = pattern;
        _mask = mask;
    }

    Tuple<Match, usize> match(Bytes bytes) const {
        usize i = 0;
        for (; i < min(bytes.len(), len()); i++) {
            if ((bytes[i] & _mask[i]) != _pattern[i])
                return {Match::NO, 0};
        }

        if (i != len())
            return {Match::PARTIAL, i};

        return {Match::YES, i};
    }

    usize len() const {
        return _pattern.len();
    }
};

struct BScan {
    Cursor<u8> _start;
    Cursor<u8> _cursor;
    u8 _bits = 0;
    u8 _bitsLen = 0;

    always_inline constexpr BScan(Bytes bytes)
        : _start(bytes),
          _cursor(bytes) {}

    always_inline constexpr void rewind() {
        _cursor = _start;
    }

    always_inline constexpr Res<> ensure(usize n) {
        if (rem() < n)
            return Error::invalidInput("out of bound");
        return Ok();
    }

    always_inline constexpr BScan& seek(usize n) {
        rewind();
        return skip(n);
    }

    always_inline constexpr usize tell() {
        return _cursor - _start;
    }

    always_inline constexpr bool ended() {
        return _cursor.ended();
    }

    always_inline constexpr usize rem() {
        return _cursor.rem();
    }

    always_inline constexpr Bytes remBytes() {
        return {_cursor.buf(), rem()};
    }

    always_inline constexpr BScan& skip(usize n) {
        n = min(n, rem());
        _cursor.next(n);
        return *this;
    }

    always_inline constexpr BScan peek(usize n) {
        BScan c{*this};
        c.skip(n);
        return c;
    }

    template <typename T>
    always_inline constexpr Res<> readTo(T* buf, usize n = 1) {
        try$(ensure(n));
        u8* b = reinterpret_cast<u8*>(buf);
        for (usize i = 0; i < sizeof(T) * n; i++) {
            b[i] = _cursor.next();
        }
        return Ok();
    }

    template <typename T>
    always_inline constexpr Res<> peekTo(T* buf, usize n = 1) {
        try$(ensure(n));
        u8* b = reinterpret_cast<u8*>(buf);
        for (usize i = 0; i < sizeof(T) * n; i++)
            b[i] = _cursor.buf()[i];

        return Ok();
    }

    template <typename T>
    always_inline constexpr Res<T> next() {
        T r{};
        try$(readTo(&r));
        return Ok(std::move(r));
    }

    template <typename T>
    always_inline constexpr Res<T> peek() {
        T r{};
        try$(peekTo(&r));
        return Ok(std::move(r));
    }

    always_inline constexpr void alignBits() {
        _bitsLen = 0;
    }

    /// Read bits in most significant bit first order.
    always_inline constexpr Res<u8> nextBitbe() {
        if (_bitsLen == 0) {
            _bits = try$(next<u8be>());
            _bitsLen = 8;
        }
        u8 r = _bits >> 7;
        _bits <<= 1;
        _bitsLen--;
        return Ok(r);
    }

    always_inline constexpr Res<u8> peekBitbe() {
        BScan c{*this};
        return c.nextBitbe();
    }

    always_inline constexpr Res<usize> nextBitsBe(usize n) {
        usize r = 0;
        for (usize i = n - 1; i < n; i--) {
            r |= try$(nextBitbe()) << i;
        }
        return Ok(r);
    }

    always_inline constexpr Res<usize> peekBitsBe(usize n) {
        BScan c{*this};
        return c.nextBitsBe(n);
    }

    always_inline constexpr Res<> skipBitsbe(usize n) {
        for (usize i = 0; i < n; i++) {
            try$(nextBitbe());
        }
        return Ok();
    }

    /// Read bits in least significant bit first order.
    always_inline constexpr Res<u8> nextBitLe() {
        if (_bitsLen == 0) {
            _bits = try$(next<u8le>());
            _bitsLen = 8;
        }
        u8 r = _bits & 1;
        _bits >>= 1;
        _bitsLen--;
        return Ok(r);
    }

    always_inline constexpr Res<u8> peekBitLe() {
        BScan c{*this};
        return c.nextBitLe();
    }

    always_inline constexpr Res<usize> nextBitsLe(usize n) {
        usize r = 0;
        for (usize i = 0; i < n; i++) {
            r |= try$(nextBitLe()) << i;
        }
        return Ok(r);
    }

    always_inline constexpr Res<usize> peekBitsLe(usize n) {
        BScan c{*this};
        return c.nextBitsLe(n);
    }

    always_inline constexpr Res<> skipBitsLe(usize n) {
        for (usize i = 0; i < n; i++) {
            try$(nextBitLe());
        }
        return Ok();
    }

    always_inline constexpr Res<Str> nextStr(usize n) {
        try$(ensure(n));
        Str s{(char const*)_cursor.buf(), n};
        _cursor.next(n);
        return Ok(s);
    }

    always_inline constexpr Res<Str> nextCStr() {
        usize n = 0;
        while (n < rem() and _cursor.buf()[n] != '\0') {
            n++;
        }
        return nextStr(n);
    }

    always_inline constexpr Res<Bytes> nextBytes(usize n) {
        try$(ensure(n));
        Bytes b{_cursor.buf(), n};
        _cursor.next(n);
        return Ok(b);
    }
};

template <typename T, usize Offset>
struct BField {
    using Type = T;
    static constexpr usize offset = Offset;
};

struct BChunk {
    Bytes _slice{};

    always_inline constexpr BChunk() = default;

    always_inline constexpr BChunk(Bytes slice) : _slice(slice) {}

    always_inline constexpr bool present() const {
        return _slice.len() > 0;
    }

    always_inline constexpr BScan begin() const {
        return _slice;
    }

    always_inline constexpr BScan begin(usize n) const {
        return begin().seek(n);
    }

    always_inline constexpr Bytes bytes() const {
        return _slice;
    }

    template <typename T>
    always_inline constexpr Res<typename T::Type> get() const {
        return begin()
            .skip(T::offset)
            .template next<typename T::Type>();
    }
};

struct BEmit {
    Writer& _writer;

    always_inline constexpr BEmit(Writer& writer)
        : _writer(writer) {}

    template <Meta::TrivialyCopyable T>
    always_inline constexpr void writeFrom(T const& v) {
        (void)_writer.write(Bytes{(u8 const*)&v, sizeof(v)});
    }

    always_inline constexpr void writeU8be(u8be v) {
        writeFrom(v);
    }

    always_inline constexpr void writeU16be(u16be v) {
        writeFrom(v);
    }

    always_inline constexpr void writeU32be(u32be v) {
        writeFrom(v);
    }

    always_inline constexpr void writeU64be(u64be v) {
        writeFrom(v);
    }

    always_inline constexpr void writeU8le(u8le v) {
        writeFrom(v);
    }

    always_inline constexpr void writeU16le(u16le v) {
        writeFrom(v);
    }

    always_inline constexpr void writeU32le(u32le v) {
        writeFrom(v);
    }

    always_inline constexpr void writeU64le(u64le v) {
        writeFrom(v);
    }

    always_inline constexpr void writeI8be(i8be v) {
        writeFrom(v);
    }

    always_inline constexpr void writeI16be(i16be v) {
        writeFrom(v);
    }

    always_inline constexpr void writeI32be(i32be v) {
        writeFrom(v);
    }

    always_inline constexpr void writeI64be(i64be v) {
        writeFrom(v);
    }

    always_inline constexpr void writeI8le(i8le v) {
        writeFrom(v);
    }

    always_inline constexpr void writeI16le(i16le v) {
        writeFrom(v);
    }

    always_inline constexpr void writeI32le(i32le v) {
        writeFrom(v);
    }

    always_inline constexpr void writeI64le(i64le v) {
        writeFrom(v);
    }

    always_inline constexpr void writeStr(Str s) {
        (void)_writer.write(Bytes{(u8 const*)s.buf(), s.len()});
    }

    always_inline constexpr void writeCStr(Str s) {
        (void)_writer.write(Bytes{(u8 const*)s.buf(), s.len()});
        (void)_writer.write(Bytes{(u8 const*)"\0", 1});
    }

    always_inline constexpr void writeBytes(Bytes b) {
        (void)_writer.write(b);
    }
};

} // namespace Karm::Io
