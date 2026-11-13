module;

#include <karm/macros>

export module Karm.Core:base.bufferBuilder;

import :base.align;
import :base.endian;
import :base.panic;
import :base.slice;
import :base.string;
import :base.vec;

namespace Karm {

export struct BufferBuilder {
    Vec<u8> _buf{};

    BufferBuilder(usize cap = 16) : _buf(cap) {}

    BufferBuilder(Vec<u8>&& buf) : _buf(std::move(buf)) {}

    void ensure(usize cap) {
        _buf.ensure(cap);
    }

    void append(Bytes bytes) {
        _buf.insertMany(_buf.len(), bytes);
    }

    template <Meta::TriviallyCopyable T>
    always_inline void appendFrom(T const& v) {
        append(Bytes{(u8 const*)&v, sizeof(v)});
    }

    void appendFill(usize count, u8 fill = 0) {
        _buf.resize(_buf.len() + count, fill);
    }

    void align(usize n, u8 fill = 0) {
        _buf.resize(alignUp(_buf.len(), n), fill);
    }

    always_inline void appendU8be(u8be v) { appendFrom(v); }

    always_inline void appendU16be(u16be v) { appendFrom(v); }

    always_inline void appendU32be(u32be v) { appendFrom(v); }

    always_inline void appendU64be(u64be v) { appendFrom(v); }

    always_inline void appendU8le(u8le v) { appendFrom(v); }

    always_inline void appendU16le(u16le v) { appendFrom(v); }

    always_inline void appendU32le(u32le v) { appendFrom(v); }

    always_inline void appendU64le(u64le v) { appendFrom(v); }

    always_inline void appendI8be(i8be v) { appendFrom(v); }

    always_inline void appendI16be(i16be v) { appendFrom(v); }

    always_inline void appendI32be(i32be v) { appendFrom(v); }

    always_inline void appendI64be(i64be v) { appendFrom(v); }

    always_inline void appendI8le(i8le v) { appendFrom(v); }

    always_inline void appendI16le(i16le v) { appendFrom(v); }

    always_inline void appendI32le(i32le v) { appendFrom(v); }

    always_inline void appendI64le(i64le v) { appendFrom(v); }

    always_inline void appendF32(f32 v) { appendFrom(v); }

    always_inline void appendF64(f64 v) { appendFrom(v); }

    void appendStr(Str s) {
        append(Bytes{(u8 const*)s.buf(), s.len()});
    }

    void appendCStr(Str s) {
        appendStr(s);
        appendU8be(0);
    }

    void patch(usize at, Bytes bytes) {
        if (at > _buf.len() or bytes.len() > _buf.len() - at) [[unlikely]]
            panic("patch out of bounds");
        copy(bytes, mutSub(_buf, at, at + bytes.len()));
    }

    template <Meta::TriviallyCopyable T>
    always_inline void patchFrom(usize at, T const& v) {
        patch(at, Bytes{(u8 const*)&v, sizeof(v)});
    }

    always_inline void patchU8be(usize at, u8be v) { patchFrom(at, v); }

    always_inline void patchU16be(usize at, u16be v) { patchFrom(at, v); }

    always_inline void patchU32be(usize at, u32be v) { patchFrom(at, v); }

    always_inline void patchU64be(usize at, u64be v) { patchFrom(at, v); }

    always_inline void patchU8le(usize at, u8le v) { patchFrom(at, v); }

    always_inline void patchU16le(usize at, u16le v) { patchFrom(at, v); }

    always_inline void patchU32le(usize at, u32le v) { patchFrom(at, v); }

    always_inline void patchU64le(usize at, u64le v) { patchFrom(at, v); }

    always_inline void patchI8be(usize at, i8be v) { patchFrom(at, v); }

    always_inline void patchI16be(usize at, i16be v) { patchFrom(at, v); }

    always_inline void patchI32be(usize at, i32be v) { patchFrom(at, v); }

    always_inline void patchI64be(usize at, i64be v) { patchFrom(at, v); }

    always_inline void patchI8le(usize at, i8le v) { patchFrom(at, v); }

    always_inline void patchI16le(usize at, i16le v) { patchFrom(at, v); }

    always_inline void patchI32le(usize at, i32le v) { patchFrom(at, v); }

    always_inline void patchI64le(usize at, i64le v) { patchFrom(at, v); }

    usize len() const {
        return _buf.len();
    }

    Bytes bytes() const lifetimebound {
        return _buf;
    }

    MutBytes mutBytes() lifetimebound {
        return _buf;
    }

    void clear() {
        _buf.trunc(0);
    }

    Vec<u8> take() {
        return std::move(_buf);
    }
};

} // namespace Karm
