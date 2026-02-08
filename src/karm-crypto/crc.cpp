module;

// https://web.mit.edu/freebsd/head/sys/libkern/crc32.c
// https://reveng.sourceforge.io/crc-catalogue/

#include <karm/macros>

export module Karm.Crypto:crc;

import Karm.Core;

namespace Karm::Crypto {

constexpr Array<u8, 9> CRC_CHECK = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};

export template <typename D, D Poly, D Init, bool RefIn, bool RefOut, D XorOut, D Residue>
struct Crc {
    using Digest = D;

    static constexpr Digest EffectivePoly = RefIn ? reverseBits(Poly) : Poly;
    static constexpr usize Width = sizeof(Digest) * 8;

    static constexpr Array<Digest, 256> TABLE = [] {
        Array<Digest, 256> res = {};
        for (auto d : range<usize>(256)) {
            Digest r = static_cast<Digest>(d);

            if constexpr (RefIn) {
                for (usize _ : range(8)) {
                    if (r & 1)
                        r = (r >> 1) ^ EffectivePoly;
                    else
                        r >>= 1;
                }
            } else {
                r <<= (Width - 8);
                for (usize _ : range(8)) {
                    if (r & (Digest(1) << (Width - 1)))
                        r = (r << 1) ^ EffectivePoly;
                    else
                        r <<= 1;
                }
            }
            res[d] = r;
        }
        return res;
    }();

    Digest _register = Init;

    constexpr Crc() = default;

    constexpr void update(u8 byte) {
        if constexpr (RefIn) {
            _register = TABLE[(_register ^ byte) & 0xFF] ^ (_register >> 8);
        } else {
            _register = TABLE[(_register >> (Width - 8)) ^ byte] ^ (_register << 8);
        }
    }

    constexpr void update(Bytes bytes) {
        auto [buf, len] = bytes;
        for (usize i = 0; i < len; ++i)
            update(buf[i]);
    }

    constexpr Digest digest() {
        Digest res = _register;
        if constexpr (RefIn != RefOut)
            res = reverseBits(res);
        return res ^ XorOut;
    }

    constexpr bool check() {
        return _register == Residue;
    }
};

#define CRC$(NAME, WIDTH, POLY, INIT, REF_IN, REF_OUT, XOR_OUT, CHECK, RESIDUE)         \
    export using Crc##NAME = Crc<WIDTH, POLY, INIT, REF_IN, REF_OUT, XOR_OUT, RESIDUE>; \
    export constexpr Crc##NAME::Digest crc##NAME(Bytes bytes) {                         \
        Crc##NAME crc;                                                                  \
        crc.update(bytes);                                                              \
        return crc.digest();                                                            \
    }                                                                                   \
    export constexpr bool crc##NAME##check(Bytes bytes) {                               \
        Crc##NAME crc;                                                                  \
        crc.update(bytes);                                                              \
        return crc.check();                                                             \
    }                                                                                   \
    static_assert(crc##NAME(CRC_CHECK) == CHECK, "CRC check failed. Check the parameters for Crc" #NAME);

CRC$(32, u32, 0x04c11db7, 0xffffffff, true, true, 0xffffffff, 0xcbf43926, 0xdebb20e3);
CRC$(32Cksum, u32, 0x04c11db7, 0x00000000, false, false, 0xffffffff, 0x765e7680, 0xc704dd7b);
CRC$(32CdRomEdc, u32, 0x8001801b, 0x00000000, true, true, 0x00000000, 0x6ec2edc4, 0x00000000);
CRC$(64Nvme, u64, 0xad93d23594c93659, 0xffffffffffffffff, true, true, 0xffffffffffffffff, 0xae8b14860a799888, 0xf310303b2b6f6e42);

} // namespace Karm::Crypto
