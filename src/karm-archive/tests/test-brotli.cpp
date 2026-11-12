#include <karm/test>

import Karm.Archive;

// Unless noted otherwise, the compressed streams below were produced by
// the reference implementation (google/brotli 1.1.0). The ones described
// as hand-assembled were put together bit by bit following RFC 7932 to pin
// down a specific code path, then checked against the reference decoder.

using namespace Karm::Literals;

namespace Karm::Archive::Tests {

test$("brotli-decompress") {
    auto testCase = [&](Bytes input, Str expected) -> Res<> {
        auto res = try$(brotliDecompress(input));
        assertEq$(res, bytes(expected));
        return Ok();
    };

    Array input0 = {
        (u8)0x8B, 0x05, 0x80, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72,
        0x6C, 0x64, 0x21, 0x03
    };
    try$(testCase(input0, "Hello World!"));

    // Mostly static dictionary references, with and without transforms
    Array input1 = {
        (u8)0x1B, 0x29, 0x01, 0x80, 0x2C, 0x12, 0x78, 0x6E, 0xD2, 0x39, 0x27, 0xF4,
        0x48, 0x35, 0xFA, 0xB2, 0x51, 0x2A, 0x2A, 0xB7, 0x0C, 0x4B, 0x2A, 0xCB,
        0x30, 0xB4, 0xBB, 0x06, 0x29, 0x63, 0xA3, 0xDB, 0x20, 0x2E, 0x6D, 0x79,
        0xE1, 0xDF, 0x76, 0x02, 0x03, 0xA4, 0xDD, 0x5F, 0xDB, 0x5D, 0x17, 0x66,
        0x18, 0x3C, 0x4C, 0xC7, 0x59, 0x6E, 0xD3, 0xA7, 0xE7, 0x49, 0x3A, 0x24,
        0xA4, 0x60, 0x03, 0xFB, 0x29, 0xF8, 0xB4, 0x18, 0xBF, 0x49, 0xA9, 0xD4,
        0x8D, 0xB6, 0x12, 0x63, 0xE9, 0x0E, 0xDD, 0xC4, 0x70, 0x00, 0x78, 0xD7,
        0xFA, 0x01, 0x56, 0x30, 0x5E, 0x16, 0xA7, 0xFC, 0x4C, 0xE7, 0xE0, 0xB5,
        0x68, 0x7E, 0x05, 0x91, 0xEE, 0x1A, 0x35, 0x4B, 0xD1, 0xED, 0x59, 0xD9,
        0x07, 0x01, 0x51, 0x98, 0xB6, 0x4A, 0xA9, 0x15, 0x87, 0xDD, 0xC3, 0xE8,
        0xF5, 0x26, 0xBB, 0x7C, 0x06
    };
    try$(testCase(input1, "The history of the internet is a story about people who wanted to share information with each other. In the beginning there were only a few computers, and most of them were at universities. Today, almost everyone has access to the network, and it has changed the way we work, learn and communicate."));

    return Ok();
}

test$("brotli-decompress-empty") {
    Array input = {(u8)0x06};
    assertEq$(try$(brotliDecompress(input)).len(), 0uz);
    return Ok();
}

test$("brotli-decompress-window-bits") {
    // The same payload behind each shape of the WBITS header
    auto testCase = [&](Bytes input) -> Res<> {
        auto res = try$(brotliDecompress(input));
        assertEq$(res, bytes("abc"s));
        return Ok();
    };

    Array wbits10 = {
        (u8)0x21, 0x08, 0x00, 0x04, 0x61, 0x62, 0x63, 0x03
    };
    try$(testCase(wbits10));

    Array wbits15 = {
        (u8)0x71, 0x08, 0x00, 0x04, 0x61, 0x62, 0x63, 0x03
    };
    try$(testCase(wbits15));

    Array wbits16 = {
        (u8)0x20, 0x00, 0x10, 0x61, 0x62, 0x63, 0x03
    };
    try$(testCase(wbits16));

    Array wbits17 = {
        (u8)0x01, 0x08, 0x00, 0x04, 0x61, 0x62, 0x63, 0x03
    };
    try$(testCase(wbits17));

    Array wbits18 = {
        (u8)0x03, 0x01, 0x80, 0x61, 0x62, 0x63, 0x03
    };
    try$(testCase(wbits18));

    Array wbits24 = {
        (u8)0x0F, 0x01, 0x80, 0x61, 0x62, 0x63, 0x03
    };
    try$(testCase(wbits24));

    return Ok();
}

test$("brotli-decompress-meta-blocks") {
    // Hand-assembled: a metadata meta-block, an uncompressed one holding
    // "Hello", an empty metadata one, then a compressed one copying 10 bytes
    // from 5 bytes back, across the meta-block boundary.
    Array input = {
        (u8)0x2C, 0x01, 0x3F, 0x3F, 0x3F, 0x20, 0x00, 0x08, 0x48, 0x65, 0x6C, 0x6C,
        0x6F, 0x06, 0x91, 0x00, 0x00, 0x00, 0x02, 0x20, 0x80, 0x09, 0x09
    };

    auto out = try$(brotliDecompress(input));
    assertEq$(out, bytes("HelloHelloHello"s));

    return Ok();
}

test$("brotli-decompress-repeated") {
    // 4096 times the same sentence, a single long overlapping copy
    Array input = {
        (u8)0x5B, 0xFF, 0xAF, 0x82, 0xC8, 0x22, 0x79, 0x5C, 0xFB, 0x5A, 0x8C, 0x42,
        0x3B, 0xF4, 0x25, 0x55, 0x19, 0x5A, 0x92, 0x99, 0xB1, 0x35, 0xC8, 0x19,
        0x9E, 0x9E, 0x0A, 0x7B, 0x4B, 0x90, 0xB9, 0x3C, 0xD8, 0x80, 0x03, 0x87,
        0x04, 0xF2, 0x36, 0x82, 0x1B, 0x74, 0x5A, 0xE1, 0xCC, 0xE1, 0x71, 0xA2,
        0x9A, 0x82, 0x11, 0xEE, 0xF1, 0x54, 0x60, 0x4F, 0x29
    };

    auto sentence = bytes("The quick brown fox jumps over the lazy dog"s);
    auto out = try$(brotliDecompress(input));
    assertEq$(out.len(), sentence.len() * 4096);
    for (usize i : urange::zeroTo(out.len())) {
        assertEq$(out[i], sentence[i % sentence.len()]);
    }

    return Ok();
}

test$("brotli-decompress-zeroes") {
    Array input = {
        (u8)0x5B, 0xFF, 0xFF, 0x83, 0x7F, 0x02, 0x20, 0x1E, 0x0B, 0x04, 0x72, 0xEF,
        0x07, 0x00
    };

    auto out = try$(brotliDecompress(input));
    assertEq$(out.len(), 262144uz);
    for (usize i : urange::zeroTo(out.len())) {
        assertEq$(out[i], 0);
    }

    return Ok();
}

test$("brotli-decompress-window-wrap") {
    // 5000 bytes through a 1KiB window (WBITS = 10), so the sliding window
    // has to wrap around several times.
    Array input = {
        (u8)0xA1, 0x38, 0x9C, 0x00, 0x20, 0xEE, 0x00, 0x66, 0x73, 0xA9, 0xCA, 0x1A,
        0x4C, 0xA0, 0xF9, 0x42, 0x59, 0x4B, 0xDB, 0xC6, 0x37, 0x2E, 0x09, 0x09,
        0xE8, 0x92, 0xD9, 0x81, 0x7E, 0x00, 0x1A, 0x04, 0x34, 0x13
    };

    auto out = try$(brotliDecompress(input));
    assertEq$(out.len(), 5000uz);
    for (usize i : urange::zeroTo(out.len())) {
        assertEq$(out[i], i * 7 % 13 + (i % 1000 == 0 ? 1 : 0));
    }

    return Ok();
}

test$("brotli-decompress-context-modes") {
    // Hand-assembled: 8 literals taken from two prefix codes of a single
    // symbol each, 'A' and 'z'. No bits are spent on the literals, so the
    // output only depends on the context id computed from the previous two
    // bytes. The four streams only differ by their context mode.
    auto testCase = [&](Bytes input, Str expected) -> Res<> {
        auto res = try$(brotliDecompress(input));
        assertEq$(res, bytes(expected));
        return Ok();
    };

    Array lsb6 = {
        (u8)0xE2, 0x00, 0x00, 0x00, 0xA1, 0x04, 0x00, 0x08, 0x48, 0x00, 0x00, 0x28,
        0x00, 0x20, 0x82, 0x42, 0x2F, 0x70, 0x09, 0x08
    };
    try$(testCase(lsb6, "AAAAAAAA"));

    Array msb6 = {
        (u8)0xE2, 0x00, 0x00, 0x40, 0xA1, 0x04, 0x00, 0x08, 0x48, 0x00, 0x00, 0x28,
        0x00, 0x20, 0x82, 0x42, 0x2F, 0x70, 0x09, 0x08
    };
    try$(testCase(msb6, "AzAzAzAz"));

    Array utf8 = {
        (u8)0xE2, 0x00, 0x00, 0x80, 0xA1, 0x04, 0x00, 0x08, 0x48, 0x00, 0x00, 0x28,
        0x00, 0x20, 0x82, 0x42, 0x2F, 0x70, 0x09, 0x08
    };
    try$(testCase(utf8, "AzAAzAAz"));

    Array sign = {
        (u8)0xE2, 0x00, 0x00, 0xC0, 0xA1, 0x04, 0x00, 0x08, 0x48, 0x00, 0x00, 0x28,
        0x00, 0x20, 0x82, 0x42, 0x2F, 0x70, 0x09, 0x08
    };
    try$(testCase(sign, "Azzzzzzz"));

    return Ok();
}

test$("brotli-decompress-block-switch") {
    // Hand-assembled: block switches in all three categories, using the
    // "previous", "next" and explicit block type codes; a literal context
    // map using zero runs and the inverse move-to-front transform; a
    // distance context map that depends on the copy length; direct
    // (NDIRECT = 2), short and implicit distance codes.
    Array input = {
        (u8)0xC2, 0x04, 0x60, 0x64, 0xB0, 0x00, 0x46, 0x14, 0xA0, 0x88, 0x02, 0x84,
        0xD0, 0x63, 0x75, 0xD9, 0x21, 0x81, 0xF8, 0xF3, 0x87, 0x72, 0x49, 0x84,
        0x85, 0x58, 0x8C, 0x85, 0x26, 0x91, 0x40, 0x44, 0x42, 0x20, 0xD4, 0xDF,
        0x6E
    };

    auto out = try$(brotliDecompress(input));
    assertEq$(out, bytes("aaaaaaabbcbcbcbcbcbcbcbbbcbbbbccccccccb"s));

    return Ok();
}

test$("brotli-decompress-distance-postfix") {
    // Hand-assembled: NPOSTFIX = 1 and NDIRECT = 4, both copies go through
    // the same distance code and only differ by its extra bit.
    Array input = {
        (u8)0xC2, 0x01, 0x00, 0x09, 0x74, 0x98, 0xD8, 0x18, 0x99, 0xD2, 0x25, 0x90,
        0xA8, 0x84, 0x2D, 0x17
    };

    auto out = try$(brotliDecompress(input));
    assertEq$(out, bytes("abcdbadcabcddca"s));

    return Ok();
}

test$("brotli-decompress-dictionary") {
    // Hand-assembled: a single copy pointing past the start of the output,
    // into the static dictionary, one stream per word transformation.
    auto testCase = [&](Bytes input, Str expected) -> Res<> {
        auto res = try$(brotliDecompress(input));
        assertEq$(res, bytes(expected));
        return Ok();
    };

    Array identity = {
        (u8)0x62, 0x00, 0x00, 0x00, 0x04, 0x40, 0x08, 0x12, 0x10
    };
    try$(testCase(identity, "time"));

    Array uppercaseFirst = {
        (u8)0x62, 0x00, 0x00, 0x00, 0x04, 0x40, 0x08, 0x12, 0x26, 0x01, 0x01
    };
    try$(testCase(uppercaseFirst, "Time"));

    Array uppercaseAll = {
        (u8)0x62, 0x00, 0x00, 0x00, 0x04, 0x40, 0x08, 0x12, 0x2A, 0x01, 0x0C
    };
    try$(testCase(uppercaseAll, "TIME"));

    Array omitFirst1 = {
        (u8)0x42, 0x00, 0x00, 0x00, 0x04, 0x40, 0x08, 0x12, 0x23, 0x01
    };
    try$(testCase(omitFirst1, "ime"));

    Array omitLast1 = {
        (u8)0x42, 0x00, 0x00, 0x00, 0x04, 0x40, 0x08, 0x12, 0x27, 0x01, 0x00
    };
    try$(testCase(omitLast1, "tim"));

    // a longer word, its copy length takes extra bits
    Array omitFirst4 = {
        (u8)0xC2, 0x00, 0x00, 0x00, 0x04, 0x40, 0x00, 0x13, 0xEA, 0x04, 0x04
    };
    try$(testCase(omitFirst4, "rmation"));

    Array omitLast9 = {
        (u8)0x22, 0x00, 0x00, 0x00, 0x04, 0x40, 0x00, 0x13, 0xEC, 0x04, 0x00
    };
    try$(testCase(omitLast9, "in"));

    Array omitLast1Suffix = {
        (u8)0xC2, 0x00, 0x00, 0x00, 0x04, 0x40, 0x08, 0x12, 0xEB, 0x11, 0x01
    };
    try$(testCase(omitLast1Suffix, "making "));

    Array prefixSuffix = {
        (u8)0x02, 0x02, 0x00, 0x00, 0x04, 0x40, 0x08, 0x12, 0x2C, 0x01, 0x09
    };
    try$(testCase(prefixSuffix, " the time of the "));

    Array nbspPrefix = {
        (u8)0xA2, 0x00, 0x00, 0x00, 0x04, 0x40, 0x08, 0x12, 0x2D, 0x01, 0x06
    };
    try$(testCase(nbspPrefix, "\u00a0time"));

    // uppercasing 2 bytes sequences flips a bit of the second byte
    Array uppercaseAllCyrillic = {
        (u8)0x62, 0x00, 0x00, 0x00, 0x04, 0x40, 0x08, 0x12, 0xEA, 0xEB, 0x0C
    };
    try$(testCase(uppercaseAllCyrillic, "ЗА"));

    // uppercasing 3 bytes sequences flips two bits of the third byte
    Array uppercaseFirstCjk = {
        (u8)0xA2, 0x00, 0x00, 0x00, 0x04, 0x40, 0x10, 0x12, 0x28, 0x9E, 0x02
    };
    try$(testCase(uppercaseFirstCjk, "丨文"));

    return Ok();
}

test$("brotli-decompress-invalid") {
    auto testCase = [&](Bytes input) -> Res<> {
        assert$(not brotliDecompress(input).has());
        return Ok();
    };

    // nothing to read at all
    try$(testCase(Bytes{}));

    // "Hello World!", cut short
    Array truncated = {
        (u8)0x8B, 0x05, 0x80, 0x48, 0x65, 0x6C, 0x6C, 0x6F
    };
    try$(testCase(truncated));

    // reserved WBITS pattern
    Array wbits = {
        (u8)0x11
    };
    try$(testCase(wbits));

    // reserved bit set in a metadata meta-block
    Array reserved = {
        (u8)0x1C
    };
    try$(testCase(reserved));

    // empty stream, but the padding bits are not zero
    Array padding = {
        (u8)0x86
    };
    try$(testCase(padding));

    // simple prefix code listing the same symbol twice
    Array duplicate = {
        (u8)0x22, 0x00, 0x00, 0x00, 0x54, 0x50, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00
    };
    try$(testCase(duplicate));

    // dictionary reference with a copy length of 2, no word is that short
    Array dictionary = {
        (u8)0x22, 0x00, 0x00, 0x00, 0x04, 0x40, 0x00, 0x12, 0x10
    };
    try$(testCase(dictionary));

    return Ok();
}

test$("brotli-compress") {
    // WBITS = 16, ISLAST, ISLASTEMPTY
    Array empty = {(u8)0x06};
    assertEq$(try$(brotliCompress(Bytes{})), empty);

    // WBITS = 16 and an empty metadata meta-block, an uncompressed
    // meta-block with MLEN = 1, then ISLAST and ISLASTEMPTY
    Array one = {(u8)0x0C, 0x00, 0x00, 0x08, 'a', 0x03};
    assertEq$(try$(brotliCompress(bytes("a"s))), one);

    return Ok();
}

test$("brotli-compress-roundtrip") {
    auto roundtrip = [&](Str input) -> Res<> {
        auto compressed = try$(brotliCompress(bytes(input)));
        auto decompressed = try$(brotliDecompress(compressed));
        assertEq$(decompressed, bytes(input));
        return Ok();
    };

    try$(roundtrip(""));
    try$(roundtrip("a"));
    try$(roundtrip("Hello World!"));
    try$(roundtrip("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
    try$(roundtrip("The history of the internet is a story about people who wanted to share information with each other. In the beginning there were only a few computers, and most of them were at universities. Today, almost everyone has access to the network, and it has changed the way we work, learn and communicate."));

    return Ok();
}

test$("brotli-compress-roundtrip-long") {
    // Sizes around the 65536 bytes the compressor puts in a meta-block
    auto roundtrip = [&](usize len) -> Res<> {
        Vec<u8> input;
        u32 x = 1;
        for (usize _ : urange::zeroTo(len)) {
            x = x * 1103515245u + 12345u;
            input.pushBack(u8(x >> 16));
        }

        auto compressed = try$(brotliCompress(input));
        auto decompressed = try$(brotliDecompress(compressed));
        assertEq$(decompressed, input);
        return Ok();
    };

    try$(roundtrip(65535));
    try$(roundtrip(65536));
    try$(roundtrip(65537));
    try$(roundtrip(200000));

    return Ok();
}

} // namespace Karm::Archive::Tests
