#include <karm/test>

import Karm.Crypto;

using namespace Karm::Literals;

namespace Karm::Crypto::Tests {

test$("crypto-crc32") {
    auto testCase = [&](Str data, u32 expected) -> Res<> {
        auto crc = crc32(bytes(data));
        assertEq$(crc, expected);
        return Ok();
    };

    try$(testCase("", 0x0));
    try$(testCase("The quick brown fox jumps over the lazy dog", 0x414FA339));
    try$(testCase("various CRC algorithms input data", 0x9BD366AE));

    return Ok();
}

test$("crypto-crc32-check") {
    assert$(crc32check(bytes(Array<u8, 4>{0, 0, 0, 0})));
    assert$(crc32check("The quick brown fox jumps over the lazy dog\x39\xA3\x4F\x41"_bytes));
    assert$(crc32check("various CRC algorithms input data\xAE\x66\xD3\x9B"_bytes));
    return Ok();
}

} // namespace Karm::Crypto::Tests
