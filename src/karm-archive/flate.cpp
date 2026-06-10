module;

#include <karm/macros>

export module Karm.Archive:flate;

import Karm.Core;
import Karm.Debug;
import Karm.Logger;

namespace Karm::Archive {

static auto debugFlate = Debug::Flag::debug("flate", "Log inflate decoding error.");

struct Huff {
    Array<u16, 16> lens = {};
    Array<u16, 288> syms = {};

    static Huff build(Slice<u8> lens) {
        Huff huff;

        Array<u16, 16> offs;
        usize count = 0;

        for (usize i = 0; i < 16; ++i)
            huff.lens[i] = 0;

        for (usize i = 0; i < lens.len(); ++i)
            huff.lens[lens[i]]++;

        huff.lens[0] = 0;

        for (usize i = 0; i < 16; ++i) {
            offs[i] = count;
            count += huff.lens[i];
        }

        for (usize i = 0; i < lens.len(); ++i) {
            if (lens[i])
                huff.syms[offs[lens[i]]++] = i;
        }

        return huff;
    }

    static constexpr Pair<Huff> buildFixed() {
        Array<u8, 288> lens;

        for (usize i = 0; i < 144; ++i)
            lens[i] = 8;
        for (usize i = 144; i < 256; ++i)
            lens[i] = 9;
        for (usize i = 256; i < 280; ++i)
            lens[i] = 7;
        for (usize i = 280; i < 288; ++i)
            lens[i] = 8;
        auto lensHuff = build(lens);

        for (usize i = 0; i < 30; ++i)
            lens[i] = 5;
        auto distsHuff = build(sub(lens, 0, 30));

        return {lensHuff, distsHuff};
    }

    static Res<Pair<Huff>> buildDynamic(Io::BitReader& r) {
        static Array<u8, 19> const CLENS = {
            16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
        };

        Array<u8, 320> lengths = {};

        usize hlit = 257 + try$(r.readBits<u8>(5));
        usize hdist = 1 + try$(r.readBits<u8>(5));
        usize hclen = 4 + try$(r.readBits<u8>(4));

        for (usize i = 0; i < hclen; ++i) {
            lengths[CLENS[i]] = try$(r.readBits<u8>(3));
        }

        Huff codes = build(sub(lengths, 0, 19));

        usize count = 0;
        while (count < hlit + hdist) {
            u16 sym = try$(codes.decode(r));
            if (sym < 16) {
                lengths[count++] = sym;
            } else if (sym < 19) {
                int rep = 0;
                int length = 0;

                if (sym == 16) {
                    rep = lengths[count - 1];
                    length = try$(r.readBits<u8>(2)) + 3;
                } else if (sym == 17) {
                    length = try$(r.readBits<u8>(3)) + 3;
                } else if (sym == 18) {
                    length = try$(r.readBits<u8>(7)) + 11;
                }

                do {
                    lengths[count++] = rep;
                    length--;
                } while (length);
            } else {
                break;
            }
        }

        Huff lens = build(sub(lengths, 0, hlit));
        Huff dists = build(sub(lengths, hlit, hlit + hdist));

        return Ok<Pair<Huff>>(lens, dists);
    }

    Res<u16> decode(Io::BitReader& r) {
        usize count = 0;
        int cur = 0;
        for (int i = 1; cur >= 0; i++) {
            cur = (cur << 1) | try$(r.readBit());
            count += lens[i];
            cur -= lens[i];
        }
        return Ok(syms[count + cur]);
    }
};

struct Window {
    Io::Writer& _w;
    Ring<u8> _r{32768};

    Res<> emit(u8 b) {
        try$(Io::putByte(_w, b));
        if (_r.rem() == 0)
            _r.popFront();
        _r.pushBack(b);
        return Ok();
    }

    // Peek the byte at distance `off` from the end of the output, where
    // a distance of 1 is the most recently emitted byte.
    Res<u8> peek(usize off) {
        if (off > _r.cap())
            return Error::invalidInput("peek outside of window");
        if (off > _r.len())
            return Ok(0);
        return Ok(_r.peekBack(off - 1));
    }
};

Res<> inflateNoCompression(Io::BitReader& r, Window& out) {
    u16 len = try$(r.readBytes<u16>(2));
    u16 nlen = try$(r.readBytes<u16>(2));

    // NOTE: The code bellow has seemingly useless `& 0xffff` because of Integer promotions
    //       (See https://en.cppreference.com/w/c/language/conversion.html#Integer_promotions)
    if ((nlen & 0xffff) != (~len & 0xffff))
        return Error::invalidData("invalid block len");

    for (usize _ : urange::zeroTo(len))
        try$(out.emit(try$(r.readByte())));

    return Ok();
}

constexpr Array<u16, 29> LENS = {
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51,
    59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};

constexpr Array<u16, 29> LEXT = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,
    4, 5, 5, 5, 5, 0
};

constexpr Array<u16, 30> DISTS = {
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385,
    513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};

constexpr Array<u16, 30> DEXT = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
    10, 11, 11, 12, 12, 13, 13
};

Res<> inflate(Io::BitReader& r, Window& out, Huff& lens, Huff& dists) {
    auto sym = try$(lens.decode(r));
    while (sym != 256) {
        if (sym < 256) {
            try$(out.emit(sym));
        } else {
            sym -= 257;
            auto len = try$(r.readBits<u32>(LEXT[sym])) + LENS[sym];
            auto dist = try$(dists.decode(r));
            if (dist >= 30)
                return Error::invalidData("bad distance symbol");
            auto off = try$(r.readBits<u32>(DEXT[dist])) + DISTS[dist];
            if (off == 0)
                return Error::invalidData("zero distance");

            for (usize _ : urange::zeroTo(len)) {
                u8 b = try$(out.peek(off));
                try$(out.emit(b));
            }
        }

        sym = try$(lens.decode(r));
    }

    return Ok();
}

export Res<> inflate(Io::BitReader& r, Io::Writer& out) {
    Window w{out};
    bool bfinal = false;
    auto [fixLens, fixDists] = Huff::buildFixed();

    while (not bfinal) {
        bfinal = try$(r.readBit());
        u8 btype = try$(r.readBits<u8>(2));
        if (btype == 0)
            try$(inflateNoCompression(r, w));
        else if (btype == 1)
            try$(inflate(r, w, fixLens, fixDists));
        else if (btype == 2) {
            auto [dynLens, dynDists] = try$(Huff::buildDynamic(r));
            try$(inflate(r, w, dynLens, dynDists));
        } else {
            logWarnIf(debugFlate, "invalid block type {}", btype);
            return Error::invalidData("invalid block type");
        }
    }

    return Ok();
}

// MARK: Deflate -----------------------------------------------------------

static constexpr usize MIN_MATCH = 3;
static constexpr usize MAX_MATCH = 258;
static constexpr usize MAX_DIST = 32768;
static constexpr usize HASH_BITS = 15;
static constexpr usize MAX_CHAIN = 128;

// Huffman codes are packed starting from the most significant bit,
// unlike everything else in the bitstream.
static Res<> writeCode(Io::BitWriter& w, u16 code, u8 len) {
    for (u8 i = len; i > 0; i--)
        try$(w.writeBit(code >> (i - 1)));
    return Ok();
}

static Res<> writeFixedSym(Io::BitWriter& w, u16 sym) {
    if (sym < 144)
        return writeCode(w, 0x30 + sym, 8);
    if (sym < 256)
        return writeCode(w, 0x190 + sym - 144, 9);
    if (sym < 280)
        return writeCode(w, sym - 256, 7);
    return writeCode(w, 0xc0 + sym - 280, 8);
}

static Res<> writeLen(Io::BitWriter& w, usize len) {
    usize sym = LENS.len() - 1;
    while (LENS[sym] > len)
        sym--;
    try$(writeFixedSym(w, 257 + sym));
    try$(w.writeBits<u32>(len - LENS[sym], LEXT[sym]));
    return Ok();
}

static Res<> writeDist(Io::BitWriter& w, usize dist) {
    usize sym = DISTS.len() - 1;
    while (DISTS[sym] > dist)
        sym--;
    try$(writeCode(w, sym, 5));
    try$(w.writeBits<u32>(dist - DISTS[sym], DEXT[sym]));
    return Ok();
}

static usize hash3(Bytes input, usize i) {
    u32 h = input[i] | (input[i + 1] << 8) | (input[i + 2] << 16);
    return (h * 0x9e3779b1u) >> (32 - HASH_BITS);
}

export Res<> deflate(Bytes input, Io::Writer& out) {
    Io::BitWriter w{out};

    // Emit a single final block using the fixed Huffman codes.
    try$(w.writeBit(1));
    try$(w.writeBits<u8>(0b01, 2));

    Vec<isize> head;
    head.resize(1uz << HASH_BITS, -1);
    Vec<isize> prev;
    prev.resize(input.len(), -1);

    auto insert = [&](usize pos) {
        if (pos + MIN_MATCH > input.len())
            return;
        usize h = hash3(input, pos);
        prev[pos] = head[h];
        head[h] = pos;
    };

    usize i = 0;
    while (i < input.len()) {
        usize bestLen = 0;
        usize bestDist = 0;

        if (i + MIN_MATCH <= input.len()) {
            usize maxLen = min(MAX_MATCH, input.len() - i);
            isize cand = head[hash3(input, i)];
            usize chain = MAX_CHAIN;
            while (cand >= 0 and chain-- > 0 and i - cand <= MAX_DIST) {
                usize len = 0;
                while (len < maxLen and input[cand + len] == input[i + len])
                    len++;
                if (len > bestLen) {
                    bestLen = len;
                    bestDist = i - cand;
                    if (len == maxLen)
                        break;
                }
                cand = prev[cand];
            }
        }

        if (bestLen >= MIN_MATCH) {
            try$(writeLen(w, bestLen));
            try$(writeDist(w, bestDist));
            for (usize j = 0; j < bestLen; j++)
                insert(i + j);
            i += bestLen;
        } else {
            try$(writeFixedSym(w, input[i]));
            insert(i);
            i++;
        }
    }

    try$(writeFixedSym(w, 256));
    try$(w.flush());

    return Ok();
}

} // namespace Karm::Archive
