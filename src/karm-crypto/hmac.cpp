export module Karm.Crypto:hmac;

import :sha2;

namespace Karm::Crypto {

// https://en.wikipedia.org/wiki/HMAC#Definition
export template <typename Hash>
struct Hmac {
    using Digest = Hash::Digest;

    Hash _inner;
    Hash _outer;

    constexpr Hmac(Bytes key) {
        Array<u8, Hash::BLOCK_SIZE> k{};
        auto [buf, len] = key;

        if (len > Hash::BLOCK_SIZE) {
            Hash hk;
            hk.update(key);
            auto d = hk.digest();
            for (usize i = 0; i < d.len(); ++i) {
                k[i] = d[i];
            }
        } else {
            for (usize i = 0; i < len; ++i) {
                k[i] = buf[i];
            }
        }

        Array<u8, Hash::BLOCK_SIZE> iKeyPad{};
        Array<u8, Hash::BLOCK_SIZE> oKeyPad{};

        for (usize i = 0; i < Hash::BLOCK_SIZE; ++i) {
            iKeyPad[i] = k[i] ^ 0x36;
            oKeyPad[i] = k[i] ^ 0x5C;
        }

        _inner.update(iKeyPad.bytes());
        _outer.update(oKeyPad.bytes());
    }

    constexpr void update(u8 byte) {
        _inner.update(byte);
    }

    constexpr void update(Bytes bytes) {
        _inner.update(bytes);
    }

    constexpr Digest digest() {
        _outer.update(_inner.digest().bytes());
        return _outer.digest();
    }
};

export using HmacSha256 = Hmac<Sha256>;
export using HmacSha512 = Hmac<Sha512>;

export constexpr HmacSha256::Digest hmacSha256(Bytes key, Bytes message) {
    HmacSha256 hmac{key};
    hmac.update(message);
    return hmac.digest();
}

export constexpr HmacSha512::Digest hmacSha512(Bytes key, Bytes message) {
    HmacSha512 hmac{key};
    hmac.update(message);
    return hmac.digest();
}

} // namespace Karm::Crypto
