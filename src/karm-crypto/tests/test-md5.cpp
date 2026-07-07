#include <karm/test>

import Karm.Crypto;

namespace Karm::Crypto::Tests {

test$("crypto-md5") {
    static constexpr Array<u8, MD5_BYTES> EXPECTED_EMPTY = {
        0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
        0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e
    };

    static constexpr Array<u8, MD5_BYTES> EXPECTED_RFC_1 = {
        0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
        0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72
    };

    static constexpr Array<u8, MD5_BYTES> EXPECTED_RFC_2 = {
        0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00,
        0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b
    };

    static constexpr Array<u8, MD5_BYTES> EXPECTED_RFC_3 = {
        0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55,
        0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a
    };

    auto testCase = [&](Str data, Array<u8, MD5_BYTES> const& expected) -> Res<> {
        auto md = md5(bytes(data));

        expectEq$(md.len(), expected.len());
        for (usize idx = 0; idx < md.len(); idx++) {
            expectEq$(md[idx], expected[idx]);
        }
        return Ok();
    };

    try$(testCase("", EXPECTED_EMPTY));
    try$(testCase("abc", EXPECTED_RFC_1));
    try$(testCase("abcdefghijklmnopqrstuvwxyz", EXPECTED_RFC_2));
    try$(testCase("12345678901234567890123456789012345678901234567890123456789012345678901234567890", EXPECTED_RFC_3));

    return Ok();
}

} // namespace Karm::Crypto::Tests