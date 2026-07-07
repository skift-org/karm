#include <karm/test>

import Karm.Crypto;

namespace Karm::Crypto::Tests {

test$("crypto-sha1") {
    static constexpr Array<u8, SHA1_BYTES> EXPECTED_EMPTY = {
        0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d,
        0x32, 0x55, 0xbf, 0xef, 0x95, 0x60, 0x18, 0x90,
        0xaf, 0xd8, 0x07, 0x09
    };

    static constexpr Array<u8, SHA1_BYTES> EXPECTED_NIST_1 = {
        0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a,
        0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c,
        0x9c, 0xd0, 0xd8, 0x9d
    };

    static constexpr Array<u8, SHA1_BYTES> EXPECTED_NIST_2 = {
        0x84, 0x98, 0x3e, 0x44, 0x1c, 0x3b, 0xd2, 0x6e,
        0xba, 0xae, 0x4a, 0xa1, 0xf9, 0x51, 0x29, 0xe5,
        0xe5, 0x46, 0x70, 0xf1
    };

    static constexpr Array<u8, SHA1_BYTES> EXPECTED_NIST_3 = {
        0x34, 0xaa, 0x97, 0x3c, 0xd4, 0xc4, 0xda, 0xa4,
        0xf6, 0x1e, 0xeb, 0x2b, 0xdb, 0xad, 0x27, 0x31,
        0x65, 0x34, 0x01, 0x6f
    };

    auto testCase = [&](Str data, Array<u8, SHA1_BYTES> const& expected) -> Res<> {
        auto sha = sha1(bytes(data));

        expectEq$(sha.len(), expected.len());
        for (usize idx = 0; idx < sha.len(); idx++) {
            expectEq$(sha[idx], expected[idx]);
        }
        return Ok();
    };

    try$(testCase("", EXPECTED_EMPTY));
    try$(testCase("abc", EXPECTED_NIST_1));
    try$(testCase("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", EXPECTED_NIST_2));

    Array<u8, 1000000> nist3;

    for (usize idx = 0; idx < nist3.len(); idx++) {
        nist3[idx] = 'a';
    }
    auto sha = sha1(nist3.bytes());
    expectEq$(sha.len(), EXPECTED_NIST_3.len());
    for (usize idx = 0; idx < sha.len(); idx++) {
        expectEq$(sha[idx], EXPECTED_NIST_3[idx]);
    }

    return Ok();
}

} // namespace Karm::Crypto::Tests