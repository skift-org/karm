module;

#include <karm/macros>

export module Karm.Archive:brotli;

import Karm.Core;
import Karm.Crypto;
import Karm.Debug;
import Karm.Logger;

import :flate;

// Brotli Compressed Data Format
// https://datatracker.ietf.org/doc/html/rfc7932

namespace Karm::Archive {

// NOTE: Defined in brotli.c
extern "C" unsigned char const _BROTLI_DICT_BUF[];
extern "C" unsigned long _BROTLI_DICT_LEN;

static auto debugBrotli = Debug::Flag::debug("brotli", "Log brotli decoding error.");

static Array<u8, 4> _MNIBBLES = {4, 5, 6, 0};
static Slice<u8> _DICT = {_BROTLI_DICT_BUF, _BROTLI_DICT_LEN};

// MARK: Tables ----------------------------------------------------------------

// 3.5. Order in which the code lengths of the code length alphabet are stored
static constexpr Array<u8, 18> _CLCL_ORDER = {
    1, 2, 3, 4, 0, 5, 17, 6, 16, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

// 6. Block count codes
static constexpr Array<u16, 26> _BLEN_BASE = {
    1, 5, 9, 13, 17, 25, 33, 41, 49, 65, 81, 97, 113, 145, 177, 209, 241,
    305, 369, 497, 753, 1265, 2289, 4337, 8433, 16625
};

static constexpr Array<u8, 26> _BLEN_EXTRA = {
    2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8, 9, 10, 11,
    12, 13, 24
};

// 5. Insert length codes
static constexpr Array<u16, 24> _ILEN_BASE = {
    0, 1, 2, 3, 4, 5, 6, 8, 10, 14, 18, 26, 34, 50, 66, 98, 130, 194, 322,
    578, 1090, 2114, 6210, 22594
};

static constexpr Array<u8, 24> _ILEN_EXTRA = {
    0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10, 12, 14,
    24
};

// 5. Copy length codes
static constexpr Array<u16, 24> _CLEN_BASE = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 18, 22, 30, 38, 54, 70, 102, 134,
    198, 326, 582, 1094, 2118
};

static constexpr Array<u8, 24> _CLEN_EXTRA = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 10,
    24
};

// 5. First insert / copy length code of each 64-wide cell of the
//    insert-and-copy alphabet
static constexpr Array<u8, 11> _IAC_INSERT = {0, 0, 0, 0, 8, 8, 0, 16, 8, 16, 16};
static constexpr Array<u8, 11> _IAC_COPY = {0, 8, 0, 8, 0, 8, 16, 0, 16, 8, 16};

// 4. Distance short codes: which of the last distances, and by how much
static constexpr Array<u8, 16> _DSHORT_INDEX = {
    0, 1, 2, 3, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1
};

static constexpr Array<i8, 16> _DSHORT_OFFSET = {
    0, 0, 0, 0, -1, 1, -2, 2, -3, 3, -1, 1, -2, 2, -3, 3
};

// 8. Static dictionary: log2 of the number of words of each length,
//    and where the words of each length start
static constexpr Array<u8, 25> _NDBITS = {
    0, 0, 0, 0, 10, 10, 11, 11, 10, 10, 10, 10, 10, 9, 9, 8, 7, 7, 8, 7, 7,
    6, 6, 5, 5
};

static constexpr Array<u32, 25> _DOFFSET = {
    0, 0, 0, 0, 0, 4096, 9216, 21504, 35840, 44032, 53248, 63488, 74752,
    87040, 93696, 100864, 104704, 106752, 108928, 113536, 115968, 118528,
    119872, 121280, 122016
};

// 7.1. Context lookup for the UTF8 mode, ASCII half. The upper half is
//      regular enough to be computed, see _contextId().
static constexpr Array<u8, 128> _LUT0 = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 4, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    8, 12, 16, 12, 12, 20, 12, 16, 24, 28, 12, 12, 32, 12, 36, 12,
    44, 44, 44, 44, 44, 44, 44, 44, 44, 44, 32, 32, 24, 40, 28, 12,
    12, 48, 52, 52, 52, 48, 52, 52, 52, 48, 52, 52, 52, 52, 52, 48,
    52, 52, 52, 52, 52, 48, 52, 52, 52, 52, 52, 24, 12, 28, 12, 12,
    12, 56, 60, 60, 60, 56, 60, 60, 60, 56, 60, 60, 60, 60, 60, 56,
    60, 60, 60, 60, 60, 56, 60, 60, 60, 60, 60, 24, 12, 28, 12, 0
};

static constexpr Array<u8, 128> _LUT1 = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
    1, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1, 1, 1, 0
};

// Appendix B. List of Word Transformations
enum : u8 {
    _ID, // Identity
    _UF, // UppercaseFirst
    _UA, // UppercaseAll
    _F1, // OmitFirst1..9
    _F2,
    _F3,
    _F4,
    _F5,
    _F6,
    _F7,
    _F8,
    _F9,
    _L1, // OmitLast1..9
    _L2,
    _L3,
    _L4,
    _L5,
    _L6,
    _L7,
    _L8,
    _L9,
};

struct _BrotliTransform {
    Str prefix;
    u8 op;
    Str suffix;
};

// clang-format off
static constexpr Array<_BrotliTransform, 121> _TRANSFORMS = {
    _BrotliTransform
    {"", _ID, ""},              {"", _ID, " "},            {" ", _ID, " "},
    {"", _F1, ""},              {"", _UF, " "},            {"", _ID, " the "},
    {" ", _ID, ""},             {"s ", _ID, " "},          {"", _ID, " of "},
    {"", _UF, ""},              {"", _ID, " and "},        {"", _F2, ""},
    {"", _L1, ""},              {", ", _ID, " "},          {"", _ID, ", "},
    {" ", _UF, " "},            {"", _ID, " in "},         {"", _ID, " to "},
    {"e ", _ID, " "},           {"", _ID, "\""},           {"", _ID, "."},
    {"", _ID, "\">"},           {"", _ID, "\n"},           {"", _L3, ""},
    {"", _ID, "]"},             {"", _ID, " for "},        {"", _F3, ""},
    {"", _L2, ""},              {"", _ID, " a "},          {"", _ID, " that "},
    {" ", _UF, ""},             {"", _ID, ". "},           {".", _ID, ""},
    {" ", _ID, ", "},           {"", _F4, ""},             {"", _ID, " with "},
    {"", _ID, "'"},             {"", _ID, " from "},       {"", _ID, " by "},
    {"", _F5, ""},              {"", _F6, ""},             {" the ", _ID, ""},
    {"", _L4, ""},              {"", _ID, ". The "},       {"", _UA, ""},
    {"", _ID, " on "},          {"", _ID, " as "},         {"", _ID, " is "},
    {"", _L7, ""},              {"", _L1, "ing "},         {"", _ID, "\n\t"},
    {"", _ID, ":"},             {" ", _ID, ". "},          {"", _ID, "ed "},
    {"", _F9, ""},              {"", _F7, ""},             {"", _L6, ""},
    {"", _ID, "("},             {"", _UF, ", "},           {"", _L8, ""},
    {"", _ID, " at "},          {"", _ID, "ly "},          {" the ", _ID, " of "},
    {"", _L5, ""},              {"", _L9, ""},             {" ", _UF, ", "},
    {"", _UF, "\""},            {".", _ID, "("},           {"", _UA, " "},
    {"", _UF, "\">"},           {"", _ID, "=\""},          {" ", _ID, "."},
    {".com/", _ID, ""},         {" the ", _ID, " of the "}, {"", _UF, "'"},
    {"", _ID, ". This "},       {"", _ID, ","},            {".", _ID, " "},
    {"", _UF, "("},             {"", _UF, "."},            {"", _ID, " not "},
    {" ", _ID, "=\""},          {"", _ID, "er "},          {" ", _UA, " "},
    {"", _ID, "al "},           {" ", _UA, ""},            {"", _ID, "='"},
    {"", _UA, "\""},            {"", _UF, ". "},           {" ", _ID, "("},
    {"", _ID, "ful "},          {" ", _UF, ". "},          {"", _ID, "ive "},
    {"", _ID, "less "},         {"", _UA, "'"},            {"", _ID, "est "},
    {" ", _UF, "."},            {"", _UA, "\">"},          {" ", _ID, "='"},
    {"", _UF, ","},             {"", _ID, "ize "},         {"", _UA, "."},
    {"\xc2\xa0", _ID, ""},      {" ", _ID, ","},           {"", _UF, "=\""},
    {"", _UA, "=\""},           {"", _ID, "ous "},         {"", _UA, ", "},
    {"", _UF, "='"},            {" ", _UF, ","},           {" ", _UA, "=\""},
    {" ", _UA, ", "},           {"", _UA, ","},            {"", _UA, "("},
    {"", _UA, ". "},            {" ", _UA, "."},           {"", _UA, "='"},
    {" ", _UA, ". "},           {" ", _UF, "=\""},         {" ", _UA, "='"},
    {" ", _UF, "='"},
};
// clang-format on

// Longest possible transformed word: 5 bytes of prefix, 24 of word, 8 of suffix
using _BrotliWord = InlineVec<u8, 40>;

// MARK: Prefix Codes ----------------------------------------------------------

// 3.2. Canonical prefix code, decoded one bit at a time.
struct _BrotliCode {
    Array<u16, 16> counts = {}; //< Number of symbols of each length
    Vec<u16> syms = {};         //< Symbols ordered by (length, value)

    // A code with a single symbol takes zero bits to decode.
    static _BrotliCode single(u16 sym) {
        _BrotliCode code;
        code.syms.pushBack(sym);
        return code;
    }

    static _BrotliCode build(Slice<u8> lens) {
        _BrotliCode code;

        for (usize i = 0; i < lens.len(); i++)
            code.counts[lens[i]]++;
        code.counts[0] = 0;

        Array<u16, 16> offs = {};
        usize total = 0;
        for (usize i = 1; i < 16; i++) {
            offs[i] = total;
            total += code.counts[i];
        }

        code.syms.resize(total);
        for (usize i = 0; i < lens.len(); i++) {
            if (lens[i])
                code.syms[offs[lens[i]]++] = i;
        }

        return code;
    }

    Res<u16> decode(Io::BitReader& r) {
        if (syms.len() == 1)
            return Ok<u16>(syms[0]);

        usize code = 0;
        usize first = 0;
        usize index = 0;
        for (usize len = 1; len < 16; len++) {
            code |= try$(r.readBit());
            usize count = counts[len];
            if (code < first + count)
                return Ok<u16>(syms[index + (code - first)]);
            index += count;
            first = (first + count) << 1;
            code <<= 1;
        }

        return Error::invalidData("invalid prefix code");
    }
};

// 3.4. Simple Prefix Codes
static Res<_BrotliCode> _readSimplePrefixCode(Io::BitReader& r, usize alphabetSize) {
    usize bits = 0;
    while ((alphabetSize - 1) >> bits)
        bits++;

    usize nsym = try$(r.readBits<u8>(2)) + 1;
    Array<u16, 4> syms = {};
    for (usize i = 0; i < nsym; i++) {
        syms[i] = try$(r.readBits<u16>(bits));
        if (syms[i] >= alphabetSize)
            return Error::invalidData("symbol outside of alphabet");
        for (usize j = 0; j < i; j++) {
            if (syms[j] == syms[i])
                return Error::invalidData("duplicate symbol in simple prefix code");
        }
    }

    if (nsym == 1)
        return Ok(_BrotliCode::single(syms[0]));

    // Symbols sharing a code length are assigned codes in increasing order.
    auto sortSyms = [&](usize start, usize end) {
        auto range = mutSub(syms, start, end);
        sort(range);
    };

    _BrotliCode code;
    if (nsym == 2) {
        // 1, 1
        code.counts[1] = 2;
        sortSyms(0, 2);
    } else if (nsym == 3) {
        // 1, 2, 2
        code.counts[1] = 1;
        code.counts[2] = 2;
        sortSyms(1, 3);
    } else if (not try$(r.readBit())) {
        // 2, 2, 2, 2
        code.counts[2] = 4;
        sortSyms(0, 4);
    } else {
        // 1, 2, 3, 3
        code.counts[1] = 1;
        code.counts[2] = 1;
        code.counts[3] = 2;
        sortSyms(2, 4);
    }

    for (u16 sym : sub(syms, 0, nsym))
        code.syms.pushBack(sym);

    return Ok(std::move(code));
}

// 3.5. Complex Prefix Codes
static Res<_BrotliCode> _readComplexPrefixCode(Io::BitReader& r, usize alphabetSize, usize hskip) {
    // Code lengths of the code length alphabet, themselves coded with
    // a fixed code: 0 = 00, 3 = 10, 4 = 01, 2 = 011, 1 = 0111, 5 = 1111
    // (written as they appear in the stream, parsed right to left).
    Array<u8, 18> clLens = {};
    isize space = 32;
    usize nonZero = 0;
    for (usize i = hskip; i < 18 and space > 0; i++) {
        u8 len;
        u8 bits = try$(r.readBits<u8>(2));
        if (bits == 0)
            len = 0;
        else if (bits == 1)
            len = 4;
        else if (bits == 2)
            len = 3;
        else if (not try$(r.readBit()))
            len = 2;
        else if (not try$(r.readBit()))
            len = 1;
        else
            len = 5;

        clLens[_CLCL_ORDER[i]] = len;
        if (len) {
            space -= 32 >> len;
            nonZero++;
        }
    }

    if (nonZero != 1 and space != 0)
        return Error::invalidData("invalid code length code");

    _BrotliCode clCode;
    if (nonZero == 1) {
        for (usize i = 0; i < 18; i++) {
            if (clLens[i])
                clCode = _BrotliCode::single(i);
        }
    } else {
        clCode = _BrotliCode::build(clLens);
    }

    // Code lengths of the actual alphabet
    Vec<u8> lens;
    lens.resize(alphabetSize);

    usize sym = 0;
    u8 prevLen = 8;
    u8 repeatLen = 0;
    usize repeat = 0;
    space = 32768;
    while (sym < alphabetSize and space > 0) {
        u16 code = try$(clCode.decode(r));
        if (code < 16) {
            repeat = 0;
            lens[sym++] = code;
            if (code) {
                prevLen = code;
                space -= 32768 >> code;
            }
            continue;
        }

        // 16: repeat the previous non-zero code length 3..6 times
        // 17: repeat a code length of zero 3..10 times
        // Back to back repeats of the same kind extend each other.
        usize extraBits = code == 16 ? 2 : 3;
        u8 newLen = code == 16 ? prevLen : 0;
        if (repeatLen != newLen) {
            repeat = 0;
            repeatLen = newLen;
        }

        usize oldRepeat = repeat;
        if (repeat > 0)
            repeat = (repeat - 2) << extraBits;
        repeat += try$(r.readBits<u8>(extraBits)) + 3;

        usize delta = repeat - oldRepeat;
        if (sym + delta > alphabetSize)
            return Error::invalidData("code length repeat outside of alphabet");

        for (usize _ : urange::zeroTo(delta))
            lens[sym++] = repeatLen;
        if (repeatLen)
            space -= delta << (15 - repeatLen);
    }

    if (space != 0)
        return Error::invalidData("incomplete or oversubscribed prefix code");

    return Ok(_BrotliCode::build(lens));
}

// 3.3. Alphabet Sizes / 3.4. / 3.5.
static Res<_BrotliCode> _readPrefixCode(Io::BitReader& r, usize alphabetSize) {
    auto hskip = try$(r.readBits<u8>(2));
    if (hskip == 1)
        return _readSimplePrefixCode(r, alphabetSize);
    return _readComplexPrefixCode(r, alphabetSize, hskip);
}

// MARK: Header fields ---------------------------------------------------------

static Res<u8> _decodeWbits(Io::BitReader& r) {
    auto b0 = try$(r.readBit());
    if (b0 == 0)
        return Ok(16);

    auto b1 = try$(r.readBits<u8>(3));
    if (b1 != 0)
        return Ok(b1 + 17);

    auto b2 = try$(r.readBits<u8>(3));
    if (b2 == 0)
        return Ok(17);

    if (b2 == 1) {
        logWarnIf(debugBrotli, "large window streams are not supported");
        return Error::invalidData("invalid wbits");
    }

    return Ok(b2 + 8);
}

// 9.2. Variable-length code for a count in 1..256.
// Shared by NBLTYPESL, NBLTYPESI, NBLTYPESD, NTREESL and NTREESD.
static Res<usize> _decodeNbltypesl(Io::BitReader& r) {
    auto b0 = try$(r.readBit());
    if (b0 == 0)
        return Ok(1);

    auto b1 = try$(r.readBits<u8>(3));
    if (b1 == 0)
        return Ok(2);

    auto b2 = try$(r.readBits<u8>(b1));
    return Ok<usize>((1u << b1) + 1 + b2);
}

// Skip any bits up to the next byte boundary, they are required to be zero.
static Res<> _alignZero(Io::BitReader& r) {
    if (try$(r.readBits<u8>(r._len)) != 0)
        return Error::invalidData("padding bits should be zero");
    return Ok();
}

// 6. Block count
static Res<usize> _readBlockCount(Io::BitReader& r, _BrotliCode& code) {
    u16 sym = try$(code.decode(r));
    usize extra = try$(r.readBits<u32>(_BLEN_EXTRA[sym]));
    return Ok<usize>(_BLEN_BASE[sym] + extra);
}

// 6. Encoding of Block-Switch Commands
// State of one of the three block categories: literals, insert-and-copy
// lengths and distances.
struct _BrotliBlocks {
    usize ntypes = 1;
    _BrotliCode typeCode = {};
    _BrotliCode countCode = {};
    usize type = 0;
    usize prevType = 1;
    usize count = 16777216;

    static Res<_BrotliBlocks> read(Io::BitReader& r) {
        _BrotliBlocks blocks;
        // read NBLTYPESi
        blocks.ntypes = try$(_decodeNbltypesl(r));
        // if NBLTYPESi >= 2
        if (blocks.ntypes >= 2) {
            // read prefix code for block types, HTREE_BTYPE_i
            blocks.typeCode = try$(_readPrefixCode(r, blocks.ntypes + 2));
            // read prefix code for block counts, HTREE_BLEN_i
            blocks.countCode = try$(_readPrefixCode(r, 26));
            // read block count, BLEN_i
            blocks.count = try$(_readBlockCount(r, blocks.countCode));
            // set block type, BTYPE_i to 0
            // initialize second-to-last and last block types to 0 and 1
            // NOTE: `type` and `prevType` defaults already cover this
        }
        // else
        //     set block type, BTYPE_i to 0
        //     set block count, BLEN_i to 16777216
        return Ok(std::move(blocks));
    }

    // Account for one symbol of this category, switching to
    // the next block if the current one is exhausted.
    Res<> advance(Io::BitReader& r) {
        if (count == 0) {
            if (ntypes < 2)
                return Error::invalidData("block count exhausted");

            // read block type using HTREE_BTYPE_i and set BTYPE_i
            usize code = try$(typeCode.decode(r));
            usize next;
            if (code == 0)
                next = prevType;
            else if (code == 1)
                next = type + 1;
            else
                next = code - 2;
            if (next >= ntypes)
                next -= ntypes;

            // save previous block type
            prevType = type;
            type = next;

            // read block count using HTREE_BLEN_i and set BLEN_i
            count = try$(_readBlockCount(r, countCode));
        }
        // decrement BLEN_i
        count--;
        return Ok();
    }
};

// 7.3. Encoding of the Context Map
static Res<Vec<u8>> _readContextMap(Io::BitReader& r, usize size, usize ntrees) {
    Vec<u8> cmap;
    cmap.resize(size);

    usize rlemax = 0;
    if (try$(r.readBit()))
        rlemax = try$(r.readBits<u8>(4)) + 1;

    auto code = try$(_readPrefixCode(r, ntrees + rlemax));

    usize i = 0;
    while (i < size) {
        usize sym = try$(code.decode(r));
        if (sym == 0) {
            cmap[i++] = 0;
        } else if (sym <= rlemax) {
            // run of zeros, the map is already zero-filled
            usize run = (1u << sym) + try$(r.readBits<u32>(sym));
            if (i + run > size)
                return Error::invalidData("context map run outside of map");
            i += run;
        } else {
            cmap[i++] = sym - rlemax;
        }
    }

    // inverse move-to-front transform
    if (try$(r.readBit())) {
        auto mtf = Array<u8, 256>::fill([](usize i) -> u8 {
            return i;
        });

        for (usize j : urange::zeroTo(size)) {
            usize index = cmap[j];
            u8 value = mtf[index];
            cmap[j] = value;
            for (; index; index--)
                mtf[index] = mtf[index - 1];
            mtf[0] = value;
        }
    }

    return Ok(std::move(cmap));
}

// 7.1. Context Modes and Context ID Lookup for Literals
static usize _contextId(u8 mode, u8 p1, u8 p2) {
    // Lut2
    auto sign = [](u8 c) -> usize {
        if (c == 0)
            return 0;
        if (c < 16)
            return 1;
        if (c < 64)
            return 2;
        if (c < 128)
            return 3;
        if (c < 192)
            return 4;
        if (c < 240)
            return 5;
        if (c < 255)
            return 6;
        return 7;
    };

    switch (mode) {
    case 0: // LSB6
        return p1 & 0x3f;

    case 1: // MSB6
        return p1 >> 2;

    case 2: { // UTF8
        // Lut0: continuation bytes map to 0..1, lead bytes to 2..3
        usize c1 = p1 < 128 ? _LUT0[p1] : (p1 < 192 ? 0 : 2) + (p1 & 1);
        // Lut1: only the lead bytes of 3+ bytes sequences stand out
        usize c2 = p2 < 128 ? _LUT1[p2] : (p2 < 224 ? 0 : 2);
        return c1 | c2;
    }

    default: // Signed
        return (sign(p1) << 3) | sign(p2);
    }
}

// MARK: Dictionary ------------------------------------------------------------

// 8. Static Dictionary / Appendix B. List of Word Transformations
static _BrotliWord _transformWord(Slice<u8> word, _BrotliTransform const& t) {
    _BrotliWord out;

    for (u8 b : bytes(t.prefix))
        out.pushBack(b);

    usize start = 0;
    usize end = word.len();
    if (t.op >= _L1) {
        usize omit = t.op - _L1 + 1;
        end -= min(omit, end);
    } else if (t.op >= _F1) {
        usize omit = t.op - _F1 + 1;
        start += min(omit, end);
    }

    usize wordStart = out.len();
    for (u8 b : sub(word, start, end))
        out.pushBack(b);

    if (t.op == _UF or t.op == _UA) {
        usize i = wordStart;
        while (i < out.len()) {
            if (out[i] < 192) {
                if (out[i] >= 'a' and out[i] <= 'z')
                    out[i] ^= 32;
                i += 1;
            } else if (out[i] < 224) {
                if (i + 1 < out.len())
                    out[i + 1] ^= 32;
                i += 2;
            } else {
                if (i + 2 < out.len())
                    out[i + 2] ^= 5;
                i += 3;
            }

            if (t.op == _UF)
                break;
        }
    }

    for (u8 b : bytes(t.suffix))
        out.pushBack(b);

    return out;
}

// MARK: Decompress ------------------------------------------------------------

export Res<> brotliDecompress(Io::BitReader& r, Io::Writer& out) {
    auto wbits = try$(_decodeWbits(r));
    // read window size
    usize windowSize = (static_cast<usize>(1) << wbits) - 16;
    // sliding window over the uncompressed data, shared with inflate
    Window win{out, Ring<u8>{windowSize}};
    // ring buffer of last distances, most recent first
    Array<usize, 4> dists = {4, 11, 15, 16};
    bool isLast = true;
    // do
    do {
        // read ISLAST bit
        isLast = try$(r.readBit());
        // if ISLAST
        if (isLast) {
            // read ISLASTEMPTY bit
            bool isLastEmpty = try$(r.readBit());
            // if ISLASTEMPTY
            if (isLastEmpty)
                // break from loop
                break;
        }
        usize mlen = 0;
        // read MNIBBLES
        auto mnibbles = _MNIBBLES[try$(r.readBits<u8>(2))];
        // if MNIBBLES is zero
        if (mnibbles == 0) {
            // verify reserved bit is zero
            auto reserved = try$(r.readBit());
            if (reserved != 0)
                return Error::invalidData("reserved bit should be zero");
            // read MSKIPLEN
            auto mskipbytes = try$(r.readBits<u8>(2));
            usize mskiplen = 0;
            if (mskipbytes != 0) {
                mskiplen = try$(r.readBits<usize>(mskipbytes * 8));
                if (mskipbytes > 1 and (mskiplen >> ((mskipbytes - 1) * 8)) == 0)
                    return Error::invalidData("mskiplen is not minimal");
                mskiplen += 1;
            }
            // skip any bits up to the next byte boundary
            try$(_alignZero(r));
            // skip MSKIPLEN bytes
            for (usize _ : urange::zeroTo(mskiplen))
                try$(r.readByte());
            // continue to the next meta-block
            continue;
        }
        // else
        else {
            // read MLEN
            mlen = try$(r.readBits<usize>(mnibbles * 4));
            if (mnibbles > 4 and (mlen >> ((mnibbles - 1) * 4)) == 0)
                return Error::invalidData("mlen is not minimal");
            mlen += 1;
        }
        // if not ISLAST
        if (not isLast) {
            // read ISUNCOMPRESSED bit
            bool isUncompressed = try$(r.readBit());
            // if ISUNCOMPRESSED
            if (isUncompressed) {
                // skip any bits up to the next byte boundary
                try$(_alignZero(r));
                // copy MLEN bytes of compressed data as literals
                for (usize _ : urange::zeroTo(mlen))
                    try$(win.emit(try$(r.readByte())));
                // continue to the next meta-block
                continue;
            }
        }
        // loop for each three block categories (i = L, I, D)
        auto blocksL = try$(_BrotliBlocks::read(r));
        auto blocksI = try$(_BrotliBlocks::read(r));
        auto blocksD = try$(_BrotliBlocks::read(r));
        // read NPOSTFIX and NDIRECT
        usize npostfix = try$(r.readBits<u8>(2));
        usize ndirect = try$(r.readBits<usize>(4)) << npostfix;
        // read array of literal context modes, CMODE[]
        Vec<u8> cmode;
        for (usize _ : urange::zeroTo(blocksL.ntypes))
            cmode.pushBack(try$(r.readBits<u8>(2)));
        // read NTREESL
        usize ntreesl = try$(_decodeNbltypesl(r));
        Vec<u8> cmapl;
        // if NTREESL >= 2
        if (ntreesl >= 2) {
            // read literal context map, CMAPL[]
            cmapl = try$(_readContextMap(r, 64 * blocksL.ntypes, ntreesl));
        }
        // else
        else {
            // fill CMAPL[] with zeros
            cmapl.resize(64 * blocksL.ntypes);
        }
        // read NTREESD
        usize ntreesd = try$(_decodeNbltypesl(r));
        Vec<u8> cmapd;
        // if NTREESD >= 2
        if (ntreesd >= 2) {
            // read distance context map, CMAPD[]
            cmapd = try$(_readContextMap(r, 4 * blocksD.ntypes, ntreesd));
        }
        // else
        else {
            // fill CMAPD[] with zeros
            cmapd.resize(4 * blocksD.ntypes);
        }
        // read array of literal prefix codes, HTREEL[]
        Vec<_BrotliCode> htreel;
        for (usize _ : urange::zeroTo(ntreesl))
            htreel.pushBack(try$(_readPrefixCode(r, 256)));
        // read array of insert-and-copy length prefix codes, HTREEI[]
        Vec<_BrotliCode> htreei;
        for (usize _ : urange::zeroTo(blocksI.ntypes))
            htreei.pushBack(try$(_readPrefixCode(r, 704)));
        // read array of distance prefix codes, HTREED[]
        Vec<_BrotliCode> htreed;
        for (usize _ : urange::zeroTo(ntreesd))
            htreed.pushBack(try$(_readPrefixCode(r, 16 + ndirect + (48 << npostfix))));
        // number of uncompressed bytes produced for this meta-block
        usize produced = 0;
        // do
        do {
            // if BLEN_I is zero
            //     read block type using HTREE_BTYPE_I and set BTYPE_I
            //     save previous block type
            //     read block count using HTREE_BLEN_I and set BLEN_I
            // decrement BLEN_I
            try$(blocksI.advance(r));
            // read insert-and-copy length symbol using HTREEI[BTYPE_I]
            usize iac = try$(htreei[blocksI.type].decode(r));
            // compute insert length, ILEN, and copy length, CLEN
            usize icode = _IAC_INSERT[iac >> 6] + ((iac >> 3) & 7);
            usize ccode = _IAC_COPY[iac >> 6] + (iac & 7);
            usize ilen = _ILEN_BASE[icode] + try$(r.readBits<u32>(_ILEN_EXTRA[icode]));
            usize clen = _CLEN_BASE[ccode] + try$(r.readBits<u32>(_CLEN_EXTRA[ccode]));
            if (ilen > mlen - produced)
                return Error::invalidData("insert length outside of meta-block");

            // loop for ILEN
            for (usize _ : urange::zeroTo(ilen)) {
                // if BLEN_L is zero
                //     read block type using HTREE_BTYPE_L and set BTYPE_L
                //     save previous block type
                //     read block count using HTREE_BLEN_L and set BLEN_L
                // decrement BLEN_L
                try$(blocksL.advance(r));
                // look up context mode CMODE[BTYPE_L]
                // compute context ID, CIDL from last two uncompressed bytes
                usize cidl = _contextId(cmode[blocksL.type], try$(win.peek(1)), try$(win.peek(2)));
                // read literal using HTREEL[CMAPL[64*BTYPE_L + CIDL]]
                u8 literal = try$(htreel[cmapl[64 * blocksL.type + cidl]].decode(r));
                // write literal to uncompressed stream
                try$(win.emit(literal));
            }
            produced += ilen;
            // if number of uncompressed bytes produced in the loop for
            // this meta-block is MLEN, then break from loop (in this
            // case the copy length is ignored and can have any value)
            if (produced == mlen)
                break;

            usize dcode = 0;
            usize distance = 0;
            // if distance code is implicit zero from insert-and-copy code
            if (iac < 128) {
                // set backward distance to the last distance
                distance = dists[0];
            }
            // else
            else {
                // if BLEN_D is zero
                //     read block type using HTREE_BTYPE_D and set BTYPE_D
                //     save previous block type
                //     read block count using HTREE_BLEN_D and set BLEN_D
                // decrement BLEN_D
                try$(blocksD.advance(r));
                // compute context ID, CIDD from CLEN
                usize cidd = clen > 4 ? 3 : clen - 2;
                // read distance code using HTREED[CMAPD[4*BTYPE_D + CIDD]]
                dcode = try$(htreed[cmapd[4 * blocksD.type + cidd]].decode(r));
                // compute distance by distance short code substitution
                if (dcode < 16) {
                    isize d = static_cast<isize>(dists[_DSHORT_INDEX[dcode]]) + _DSHORT_OFFSET[dcode];
                    if (d <= 0) {
                        logWarnIf(debugBrotli, "invalid distance {} from short code {}", d, dcode);
                        return Error::invalidData("invalid distance");
                    }
                    distance = d;
                } else if (dcode < 16 + ndirect) {
                    distance = dcode - 15;
                } else {
                    usize code = dcode - ndirect - 16;
                    usize ndistbits = 1 + (code >> (npostfix + 1));
                    usize dextra = try$(r.readBits<u32>(ndistbits));
                    usize hcode = code >> npostfix;
                    usize lcode = code & ((1u << npostfix) - 1);
                    usize offset = ((2 + (hcode & 1)) << ndistbits) - 4;
                    distance = ((offset + dextra) << npostfix) + lcode + ndirect + 1;
                }
            }

            // the window holds at most windowSize bytes, so this is
            // min(windowSize, number of bytes produced so far)
            usize maxDistance = win._r.len();
            // if distance is less than the max allowed distance plus one
            if (distance <= maxDistance) {
                // if distance code is not zero,
                // and distance is not a static dictionary reference,
                if (dcode != 0) {
                    // push distance to the ring buffer of last distances
                    dists = {distance, dists[0], dists[1], dists[2]};
                }

                if (clen > mlen - produced)
                    return Error::invalidData("copy length outside of meta-block");

                // move backwards distance bytes in the uncompressed data,
                // and copy CLEN bytes from this position to
                // the uncompressed stream
                for (usize _ : urange::zeroTo(clen))
                    try$(win.emit(try$(win.peek(distance))));
                produced += clen;
            }
            // else
            else {
                // look up the static dictionary word, transform the word as
                // directed, and copy the result to the uncompressed stream
                if (clen < 4 or clen > 24) {
                    logWarnIf(debugBrotli, "distance {} outside of window {}, but no dictionary word of length {}", distance, maxDistance, clen);
                    return Error::invalidData("invalid dictionary word length");
                }

                usize wordId = distance - maxDistance - 1;
                usize index = wordId & ((1u << _NDBITS[clen]) - 1);
                usize transformId = wordId >> _NDBITS[clen];
                if (transformId >= _TRANSFORMS.len()) {
                    logWarnIf(debugBrotli, "invalid dictionary transform {}", transformId);
                    return Error::invalidData("invalid dictionary transform");
                }

                usize offset = _DOFFSET[clen] + index * clen;
                if (offset + clen > _DICT.len())
                    return Error::invalidData("dictionary word outside of dictionary");

                auto word = _transformWord(sub(_DICT, offset, offset + clen), _TRANSFORMS[transformId]);
                if (word.len() > mlen - produced)
                    return Error::invalidData("dictionary word outside of meta-block");

                for (usize i : urange::zeroTo(word.len()))
                    try$(win.emit(word[i]));
                produced += word.len();
            }
            // while number of uncompressed bytes for this meta-block < MLEN
        } while (produced < mlen);
        // while not ISLAST
    } while (not isLast);

    // what's left of the last byte is padding
    try$(_alignZero(r));

    return Ok();
}

export Res<> brotliDecompress(Io::Reader& reader, Io::Writer& out) {
    Io::BitReader bits{reader};
    return brotliDecompress(bits, out);
}

export Res<> brotliDecompress(Bytes bytes, Io::Writer& out) {
    Io::BufReader reader{bytes};
    return brotliDecompress(reader, out);
}

export Res<Vec<u8>> brotliDecompress(Bytes bytes) {
    Io::BufferWriter w;
    try$(brotliDecompress(bytes, w));
    return Ok(w.take());
}

// MARK: Compress --------------------------------------------------------------

// 11.1. Trivial Compressor
export Res<> brotliCompress(Bytes bytes, Io::Writer& out) {
    if (isEmpty(bytes)) {
        // WBITS = 16, ISLAST, ISLASTEMPTY
        try$(Io::putByte(out, 6));
        return Ok();
    }

    // WBITS = 16, followed by an empty metadata meta-block to
    // get to a byte boundary
    try$(Io::putByte(out, 12));

    usize i;
    for (i = 0; i + 65535 < bytes.len(); i += 65536) {
        try$(Io::putByte(out, 248));
        try$(Io::putByte(out, 255));
        try$(Io::putByte(out, 15));
        try$(out.write(sub(bytes, i, i + 65536)));
    }

    if (i < bytes.len()) {
        usize r = bytes.len() - i - 1;
        try$(Io::putByte(out, (r & 31) << 3));
        try$(Io::putByte(out, r >> 5));
        try$(Io::putByte(out, 8 + (r >> 13)));
        try$(out.write(sub(bytes, i, i + r + 1)));
    }

    // ISLAST, ISLASTEMPTY
    try$(Io::putByte(out, 3));

    return Ok();
}

export Res<Vec<u8>> brotliCompress(Bytes bytes) {
    Io::BufferWriter w;
    try$(brotliCompress(bytes, w));
    return Ok(w.take());
}

} // namespace Karm::Archive