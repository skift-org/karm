module;

#include <karm/macros>

export module Karm.Core:io.bscan;

import :base.endian;
import :base.vec;
import :io.aton;
import :io.traits;

namespace Karm::Io {

// Inspired by https://github.com/citizenfx/fivem/blob/master/code/client/shared/Hooking.Patterns.h#L41
export struct BPattern {
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

export struct BScan {
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

    always_inline constexpr Res<> seek(usize n) {
        if (n > _start.rem()) [[unlikely]]
            return Error::unexpectedEof("seek out of bounds");
        rewind();
        _cursor.next(n);
        return Ok();
    }

    always_inline constexpr usize tell() const {
        return _cursor - _start;
    }

    always_inline constexpr bool ended() const {
        return _cursor.ended();
    }

    always_inline constexpr usize rem() const {
        return _cursor.rem();
    }

    always_inline constexpr Bytes remBytes() const {
        return {_cursor.buf(), rem()};
    }

    /// Fails if fewer than `n` bytes remain.
    always_inline constexpr Res<> ensure(usize n) const {
        if (n > rem()) [[unlikely]]
            return Error::unexpectedEof("not enough data");
        return Ok();
    }

    always_inline constexpr Res<> skip(usize n) {
        if (n > rem()) [[unlikely]]
            return Error::unexpectedEof("skip out of bounds");
        _cursor.next(n);
        return Ok();
    }

    always_inline constexpr Res<BScan> peek(usize n) const {
        BScan c{*this};
        try$(c.skip(n));
        return Ok(c);
    }

    template <typename T>
    always_inline constexpr Res<> peekTo(T* buf, usize n = 1) const {
        if (n > rem() / sizeof(T)) [[unlikely]]
            return Error::unexpectedEof("read out of bounds");

        u8* b = reinterpret_cast<u8*>(buf);
        for (usize i = 0; i < sizeof(T) * n; i++)
            b[i] = _cursor.buf()[i];

        return Ok();
    }

    template <typename T>
    always_inline constexpr Res<> readTo(T* buf, usize n = 1) {
        try$(peekTo(buf, n));
        _cursor.next(sizeof(T) * n);
        return Ok();
    }

    template <typename T>
    always_inline constexpr Res<T> next() {
        T r{};
        try$(readTo(&r));
        return Ok(r);
    }

    template <typename T>
    always_inline constexpr Res<T> nextBe() {
        Be<T> r{};
        try$(readTo(&r));
        return Ok(static_cast<T>(r));
    }

    template <typename T>
    always_inline constexpr Res<T> nextLe() {
        Le<T> r{};
        try$(readTo(&r));
        return Ok(static_cast<T>(r));
    }

    template <typename T>
    always_inline constexpr Res<T> peek() const {
        T r{};
        try$(peekTo(&r));
        return Ok(r);
    }

    template <typename T>
    always_inline constexpr Res<T> peekBe() const {
        Be<T> r{};
        try$(peekTo(&r));
        return Ok(static_cast<T>(r));
    }

    template <typename T>
    always_inline constexpr Res<T> peekLe() const {
        Le<T> r{};
        try$(peekTo(&r));
        return Ok(static_cast<T>(r));
    }

    template <typename T>
    always_inline constexpr Res<Slice<T>> peekSlice(usize n) const {
        static_assert(alignof(T) == 1, "records must be packed");
        static_assert(Meta::TriviallyCopyable<T>, "records must be trivially copyable");

        if (n > rem() / sizeof(T)) [[unlikely]]
            return Error::unexpectedEof("slice out of bounds");
        return Ok(Slice<T>{reinterpret_cast<T const*>(_cursor.buf()), n});
    }

    template <typename T>
    always_inline constexpr Res<Slice<T>> nextSlice(usize n) {
        auto res = try$(peekSlice<T>(n));
        _cursor.next(sizeof(T) * n);
        return Ok(res);
    }

    always_inline constexpr void alignBits() {
        _bitsLen = 0;
    }

    always_inline constexpr Res<> align(usize n) {
        return skip((n - (tell() % n)) % n);
    }

    /// Read bits in most significant bit first order.
    always_inline constexpr Res<u8> nextBitbe() {
        if (_bitsLen == 0) {
            _bits = try$(nextU8be());
            _bitsLen = 8;
        }
        u8 r = _bits >> 7;
        _bits <<= 1;
        _bitsLen--;
        return Ok(r);
    }

    always_inline constexpr Res<u8> peekBitbe() const {
        BScan c{*this};
        return c.nextBitbe();
    }

    always_inline constexpr Res<usize> nextBitsbe(usize n) {
        usize r = 0;
        for (usize i = n - 1; i < n; i--) {
            r |= static_cast<usize>(try$(nextBitbe())) << i;
        }
        return Ok(r);
    }

    always_inline constexpr Res<usize> peekBitsbe(usize n) const {
        BScan c{*this};
        return c.nextBitsbe(n);
    }

    always_inline constexpr Res<> skipBitsbe(usize n) {
        for (usize i = 0; i < n; i++) {
            try$(nextBitbe());
        }
        return Ok();
    }

    /// Read bits in least significant bit first order.
    always_inline constexpr Res<u8> nextBitle() {
        if (_bitsLen == 0) {
            _bits = try$(nextU8le());
            _bitsLen = 8;
        }
        u8 r = _bits & 1;
        _bits >>= 1;
        _bitsLen--;
        return Ok(r);
    }

    always_inline constexpr Res<u8> peekBitle() const {
        BScan c{*this};
        return c.nextBitle();
    }

    always_inline constexpr Res<usize> nextBitsle(usize n) {
        usize r = 0;
        for (usize i = 0; i < n; i++) {
            r |= static_cast<usize>(try$(nextBitle())) << i;
        }
        return Ok(r);
    }

    always_inline constexpr Res<usize> peekBitsle(usize n) const {
        BScan c{*this};
        return c.nextBitsle(n);
    }

    always_inline constexpr Res<> skipBitsle(usize n) {
        for (usize i = 0; i < n; i++) {
            try$(nextBitle());
        }
        return Ok();
    }

    always_inline constexpr Res<u8> nextU8be() { return nextBe<u8>(); }

    always_inline constexpr Res<u16> nextU16be() { return nextBe<u16>(); }

    always_inline constexpr Res<u32> nextU32be() { return nextBe<u32>(); }

    always_inline constexpr Res<u64> nextU64be() { return nextBe<u64>(); }

    always_inline constexpr Res<u8> nextU8le() { return nextLe<u8>(); }

    always_inline constexpr Res<u16> nextU16le() { return nextLe<u16>(); }

    always_inline constexpr Res<u32> nextU32le() { return nextLe<u32>(); }

    always_inline constexpr Res<u64> nextU64le() { return nextLe<u64>(); }

    always_inline constexpr Res<i8> nextI8be() { return nextBe<i8>(); }

    always_inline constexpr Res<i16> nextI16be() { return nextBe<i16>(); }

    always_inline constexpr Res<i32> nextI32be() { return nextBe<i32>(); }

    always_inline constexpr Res<i64> nextI64be() { return nextBe<i64>(); }

    always_inline constexpr Res<i8> nextI8le() { return nextLe<i8>(); }

    always_inline constexpr Res<i16> nextI16le() { return nextLe<i16>(); }

    always_inline constexpr Res<i32> nextI32le() { return nextLe<i32>(); }

    always_inline constexpr Res<i64> nextI64le() { return nextLe<i64>(); }

    always_inline constexpr Res<f32> nextF32() { return nextLe<f32>(); }

    always_inline constexpr Res<f64> nextF64() { return nextLe<f64>(); }

    always_inline constexpr Res<u8> peekU8be() const { return peekBe<u8>(); }

    always_inline constexpr Res<u16> peekU16be() const { return peekBe<u16>(); }

    always_inline constexpr Res<u32> peekU32be() const { return peekBe<u32>(); }

    always_inline constexpr Res<u64> peekU64be() const { return peekBe<u64>(); }

    always_inline constexpr Res<u8> peekU8le() const { return peekLe<u8>(); }

    always_inline constexpr Res<u16> peekU16le() const { return peekLe<u16>(); }

    always_inline constexpr Res<u32> peekU32le() const { return peekLe<u32>(); }

    always_inline constexpr Res<u64> peekU64le() const { return peekLe<u64>(); }

    always_inline constexpr Res<i8> peekI8be() const { return peekBe<i8>(); }

    always_inline constexpr Res<i16> peekI16be() const { return peekBe<i16>(); }

    always_inline constexpr Res<i32> peekI32be() const { return peekBe<i32>(); }

    always_inline constexpr Res<i64> peekI64be() const { return peekBe<i64>(); }

    always_inline constexpr Res<i8> peekI8le() const { return peekLe<i8>(); }

    always_inline constexpr Res<i16> peekI16le() const { return peekLe<i16>(); }

    always_inline constexpr Res<i32> peekI32le() const { return peekLe<i32>(); }

    always_inline constexpr Res<i64> peekI64le() const { return peekLe<i64>(); }

    always_inline constexpr Res<Str> nextStr(usize n) {
        if (n > rem()) [[unlikely]]
            return Error::unexpectedEof("string out of bounds");
        Str s{(char const*)_cursor.buf(), n};
        _cursor.next(n);
        return Ok(s);
    }

    always_inline constexpr Res<Str> nextCStr(usize len = Limits<usize>::MAX) {
        usize n = 0;
        while (n < rem() and n < len and _cursor.buf()[n] != '\0')
            n++;

        if (n < rem() and n < len) {
            auto res = try$(nextStr(n));
            try$(skip(1));
            return Ok(res);
        }

        if (n == len)
            return nextStr(n);

        return Error::unexpectedEof("unterminated string");
    }

    always_inline constexpr Res<Bytes> nextBytes(usize n) {
        if (n > rem()) [[unlikely]]
            return Error::unexpectedEof("bytes out of bounds");
        Bytes b{_cursor.buf(), n};
        _cursor.next(n);
        return Ok(b);
    }

    always_inline constexpr Res<> copyTo(MutBytes dst) const {
        if (dst.len() < rem()) [[unlikely]]
            return Error::invalidInput("destination too small");

        for (usize i = 0; i < rem(); i++) {
            dst[i] = _cursor.buf()[i];
        }
        return Ok();
    }
};

export template <typename T, usize Offset>
struct BField {
    using Type = T;
    static constexpr usize offset = Offset;
};

export struct BChunk {
    Bytes _slice{};

    always_inline constexpr BChunk() = default;

    always_inline constexpr BChunk(Bytes slice) : _slice(slice) {}

    always_inline constexpr bool present() const {
        return _slice.len() > 0;
    }

    always_inline constexpr BScan begin() const {
        return _slice;
    }

    always_inline constexpr Res<BScan> begin(usize offset) const {
        return begin().peek(offset);
    }

    always_inline constexpr Bytes bytes() const {
        return _slice;
    }

    template <typename T>
    always_inline constexpr Res<typename T::Type> get() const {
        typename T::Type r{};
        auto s = try$(begin(T::offset));
        try$(s.readTo(&r));
        return Ok(r);
    }
};

export struct BEmit {
    Io::Writer& _writer;
    usize _pos = 0;

    always_inline constexpr BEmit(Io::Writer& writer)
        : _writer(writer) {}

    always_inline constexpr usize tell() const {
        return _pos;
    }

    always_inline constexpr Res<> align(usize n) {
        while (_pos % n)
            try$(writeU8be(0));
        return Ok();
    }

    template <Meta::TriviallyCopyable T>
    always_inline constexpr Res<> writeFrom(T const& v) {
        return writeBytes(Bytes{(u8 const*)&v, sizeof(v)});
    }

    always_inline constexpr Res<> writeU8be(u8be v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeU16be(u16be v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeU32be(u32be v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeU64be(u64be v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeU8le(u8le v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeU16le(u16le v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeU32le(u32le v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeU64le(u64le v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeI8be(i8be v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeI16be(i16be v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeI32be(i32be v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeI64be(i64be v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeI8le(i8le v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeI16le(i16le v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeI32le(i32le v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeI64le(i64le v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeF32(f32 v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeF64(f64 v) {
        return writeFrom(v);
    }

    always_inline constexpr Res<> writeStr(Str s) {
        return writeBytes(Bytes{(u8 const*)s.buf(), s.len()});
    }

    always_inline constexpr Res<> writeCStr(Str s) {
        try$(writeStr(s));
        return writeBytes(Bytes{(u8 const*)"\0", 1});
    }

    always_inline constexpr Res<> writeBytes(Bytes b) {
        _pos += try$(_writer.write(b));
        return Ok();
    }
};

} // namespace Karm::Io
