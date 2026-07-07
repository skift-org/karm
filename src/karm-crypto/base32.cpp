module;
#include <karm/macros>
export module Karm.Crypto:base32;

import Karm.Core;

namespace Karm::Crypto {

// clang-format off
static constexpr Array _MAP32 = {
    (u8)'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7'};

static constexpr Array _MAP32HEX = {
    (u8)'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V'};
// clang-format on

struct Base32Props {
    // Use the RFC 4648 §7 "Extended Hex" alphabet (0-9A-V) instead of
    // the RFC 4648 §6 standard alphabet (A-Z2-7).
    bool hex = false;
};

export Res<> base32Decode(Io::SScan& s, Io::Writer& out, Base32Props props = {}) {
    auto const& map = props.hex ? _MAP32HEX : _MAP32;

    u8 i = 0;
    Array<u8, 8> buf = {};
    while (not s.ended()) {
        Rune r = s.next();

        if (r >= 'a' and r <= 'z')
            r -= ('a' - 'A');

        u8 k = 0;
        while (k < 32 and map[k] != r)
            k++;
        buf[i++] = k;
        if (i != 8)
            continue;

        try$(Io::putByte(out, (buf[0] << 3) | (buf[1] >> 2)));
        if (buf[3] != 32)
            try$(Io::putByte(out, ((buf[1] & 0x3) << 6) | (buf[2] << 1) | (buf[3] >> 4)));
        if (buf[4] != 32)
            try$(Io::putByte(out, ((buf[3] & 0xF) << 4) | (buf[4] >> 1)));
        if (buf[6] != 32)
            try$(Io::putByte(out, ((buf[4] & 0x1) << 7) | (buf[5] << 2) | (buf[6] >> 3)));
        if (buf[7] != 32)
            try$(Io::putByte(out, ((buf[6] & 0x7) << 5) | buf[7]));
        i = 0;
    }
    return Ok();
}

export Res<Vec<u8>> base32Decode(Str str, Base32Props props = {}) {
    Io::SScan s{str};
    Io::BufferWriter bw;
    try$(base32Decode(s, bw, props));
    return Ok(bw.take());
}

export constexpr usize base32EncodedLen(usize n) {
    return ((n + 4) / 5) * 8;
}

export Res<> base32Encode(Io::Reader& in, Io::Emit& e, Base32Props props = {}) {
    auto const& map = props.hex ? _MAP32HEX : _MAP32;

    Array<u8, Io::DEFAULT_BUFFER_SIZE> inBuf;
    Array<u8, 5> quintet;
    usize quintetLen = 0;

    auto flushQuintet = [&](usize len) -> Res<> {
        Array<char, 8> out;

        u8 b0 = quintet[0];
        u8 b1 = len > 1 ? quintet[1] : 0;
        u8 b2 = len > 2 ? quintet[2] : 0;
        u8 b3 = len > 3 ? quintet[3] : 0;
        u8 b4 = len > 4 ? quintet[4] : 0;

        out[0] = map[(b0 >> 3) & 0x1F];
        out[1] = map[((b0 & 0x7) << 2) | (b1 >> 6)];
        out[2] = len > 1 ? map[(b1 >> 1) & 0x1F] : '=';
        out[3] = len > 1 ? map[((b1 & 0x1) << 4) | (b2 >> 4)] : '=';
        out[4] = len > 2 ? map[((b2 & 0xF) << 1) | (b3 >> 7)] : '=';
        out[5] = len > 3 ? map[(b3 >> 2) & 0x1F] : '=';
        out[6] = len > 3 ? map[((b3 & 0x3) << 3) | (b4 >> 5)] : '=';
        out[7] = len > 4 ? map[b4 & 0x1F] : '=';

        Str str = sub(out);
        try$(e.writeStr(str));
        return Ok();
    };

    for (;;) {
        usize n = try$(in.read(inBuf));
        if (n == 0)
            break;
        for (usize i = 0; i < n; ++i) {
            quintet[quintetLen++] = inBuf[i];
            if (quintetLen == 5) {
                try$(flushQuintet(5));
                quintetLen = 0;
            }
        }
    }
    if (quintetLen)
        try$(flushQuintet(quintetLen));

    return Ok();
}

export String base32Encode(Bytes in, Base32Props props = {}) {
    Io::BufReader br = in;
    Io::StringWriter sw{base32EncodedLen(in.len())};
    Io::Emit e{sw};
    base32Encode(br, e, props).unwrap();
    return sw.take();
}

} // namespace Karm::Crypto