module;

#include <karm/macros>

export module Karm.Crypto:hex;

import Karm.Core;

namespace Karm::Crypto {

export Res<> hexEncode(Bytes bytes, Io::TextWriter& out) {
    for (auto b : bytes) {
        try$(Io::format(out, "{:02x}"s, b));
    }
    return Ok();
}

export Res<String> hexEncode(Bytes bytes) {
    Io::StringWriter out;
    try$(hexEncode(bytes, out));
    return Ok(out.str());
}

export Res<Vec<u8>> hexDecode(Io::SScan& s) {
    Vec<u8> out;
    while (not s.ended())
        out.pushBack(try$(Io::atou(s.slice(2), {.base = 16})));
    return Ok(std::move(out));
}

export Res<Vec<u8>> hexDecode(Str str) {
    Io::SScan s{str};
    return hexDecode(s);
}

export Res<> hexDecode(Io::SScan& s, MutBytes out) {
    for (auto& b : out)
        b = try$(Io::atou(s.slice(2), {.base = 16}));
    return Ok();
}

export Res<> hexDecode(Str str, MutBytes out) {
    Io::SScan s{str};
    return hexDecode(s, out);
}

} // namespace Karm::Crypto
