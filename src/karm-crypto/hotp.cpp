export module Karm.Crypto:hotp;

import Karm.Core;

import :hmac;

using namespace Karm::Literals;
using namespace Karm::Fmt::Literals;

namespace Karm::Crypto {

export template <Meta::Contains<Sha1, Sha256, Sha512> Hash>
String hotp(Bytes key, u64 counter, u8 ndigit) {
    auto hash = hmac<Hash>(key, u64be(counter).bytes());
    u8 off = hash[hash.len() - 1] & 0xf;
    u32 bin = unionCast<u32be>(Array<u8, 4>::from(sub(hash, off, off + 4)));
    u32 otp = (bin & 0x7fffffff) % Math::pow<u32>(10, ndigit);
    return Io::format("{{:0{}}}"_f(ndigit), otp);
}

export template <Meta::Contains<Sha1, Sha256, Sha512> Hash>
String totp(Bytes key, UtcTime time, u8 ndigit, Duration step = 30_s) {
    u64 counter = time.val() / step.val();
    return hotp<Hash>(key, counter, ndigit);
}

} // namespace Karm::Crypto