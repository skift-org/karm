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

} // namespace Karm::Crypto