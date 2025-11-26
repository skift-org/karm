module;

#include <karm-core/macros.h>

export module Karm.Crypto:base64;

import Karm.Core;

namespace Karm::Crypto {

// clang-format off
constexpr Array _MAP = {
    (u8)'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
// clang-format on

struct Base64Props {
    bool urlEncoded = false;
};

export Res<> base64Decode(Io::SScan& s, Io::Writer& out, Base64Props props = {}) {
    u8 i = 0;
    Array<u8, 4> buf;

    while (not s.ended()) {
        Rune r;
        if (props.urlEncoded and
            s.peek() == '%' and
            isAsciiHexDigit(s.peek(1)) and
            isAsciiHexDigit(s.peek(1))) {
            s.next();
            r = Io::atou(s.slice(2), {.base = 16}).unwrap();
        } else {
            r = s.next();
        }

        u8 k = 0;
        while (k < 64 and _MAP[k] != r)
            k++;
        buf[i++] = k;

        if (i != 4)
            continue;
        try$(Io::putByte(out, (buf[0] << 2) + (buf[1] >> 4)));
        if (buf[2] != 64)
            try$(Io::putByte(out, (buf[1] << 4) + (buf[2] >> 2)));
        if (buf[3] != 64)
            try$(Io::putByte(out, (buf[2] << 6) + buf[3]));
        i = 0;
    }

    return Ok();
}

export constexpr usize base64EncodedLen(usize n) {
    return ((n + 2) / 3) * 4;
}

export Res<> base64Encode(Io::Reader& in, Io::Emit& e) {
    Array<u8, 1024> inBuf;
    Array<u8, 3> triple;
    usize tripleLen = 0;

    auto flushTriple = [&](usize len) -> Res<> {
        Array<char, 4> out;

        switch (len) {
        case 3: {
            u8 b0 = triple[0];
            u8 b1 = triple[1];
            u8 b2 = triple[2];

            out[0] = _MAP[(b0 >> 2) & 0x3F];
            out[1] = _MAP[((b0 & 0x03) << 4) | (b1 >> 4)];
            out[2] = _MAP[((b1 & 0x0F) << 2) | (b2 >> 6)];
            out[3] = _MAP[b2 & 0x3F];
            break;
        }

        case 2: {
            u8 b0 = triple[0];
            u8 b1 = triple[1];

            out[0] = _MAP[(b0 >> 2) & 0x3F];
            out[1] = _MAP[((b0 & 0x03) << 4) | (b1 >> 4)];
            out[2] = _MAP[(b1 & 0x0F) << 2];
            out[3] = '=';
            break;
        }

        case 1: {
            u8 b0 = triple[0];

            out[0] = _MAP[(b0 >> 2) & 0x3F];
            out[1] = _MAP[(b0 & 0x03) << 4];
            out[2] = '=';
            out[3] = '=';
            break;
        }

        default:
            return Ok();
        }

        Str str = sub(out);
        try$(e.writeStr(str));
        return Ok();
    };

    for (;;) {
        usize n = try$(in.read(inBuf));
        if (n == 0)
            break;

        for (usize i = 0; i < n; ++i) {
            triple[tripleLen++] = inBuf[i];

            if (tripleLen == 3) {
                try$(flushTriple(3));
                tripleLen = 0;
            }
        }
    }

    if (tripleLen)
        try$(flushTriple(tripleLen));

    return Ok();
}

export String base64Encode(Bytes in) {
    Io::BufReader br = in;
    Io::StringWriter sw{base64EncodedLen(in.len())};
    Io::Emit e{sw};
    base64Encode(br, e).unwrap();
    return sw.take();
}

} // namespace Karm::Crypto
