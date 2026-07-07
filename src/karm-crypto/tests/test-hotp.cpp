#include <karm/test>

import Karm.Crypto;

using namespace Karm::Literals;

namespace Karm::Crypto::Tests {

test$("crypto-hotp") {
    // Appendix D - HOTP Algorithm: Test Values
    // https://datatracker.ietf.org/doc/html/rfc4226#appendix-B.3
    static Str SECRET = "\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x30";
    static Array<Str, 10> VECTOR = {
        "755224", "287082", "359152", "969429", "338314",
        "254676", "287922", "162583", "399871", "520489"
    };
    for (u64 counter = 0; counter < VECTOR.len(); counter++)
        expectEq$(hotp<Sha1>(bytes(SECRET), counter, 6), VECTOR[counter]);
    return Ok();
}

test$("crypto-totp") {
    // Appendix B.  Test Vectors
    // https://datatracker.ietf.org/doc/html/rfc6238#appendix-B
    static Str SECRET_SHA1 = "12345678901234567890";
    static Str SECRET_SHA256 = "12345678901234567890123456789012";
    static Str SECRET_SHA512 = "1234567890123456789012345678901234567890123456789012345678901234";

    // T = 0000000000000001, 1970-01-01 00:00:59
    expectEq$(totp<Sha1>(bytes(SECRET_SHA1), UtcTime::epoch() + 59_s, 8), "94287082"s);
    expectEq$(totp<Sha256>(bytes(SECRET_SHA256), UtcTime::epoch() + 59_s, 8), "46119246"s);
    expectEq$(totp<Sha512>(bytes(SECRET_SHA512), UtcTime::epoch() + 59_s, 8), "90693936"s);

    // T = 00000000023523EC, 2005-03-18 01:58:29
    expectEq$(totp<Sha1>(bytes(SECRET_SHA1), UtcTime::epoch() + 1111111109_s, 8), "07081804"s);
    expectEq$(totp<Sha256>(bytes(SECRET_SHA256), UtcTime::epoch() + 1111111109_s, 8), "68084774"s);
    expectEq$(totp<Sha512>(bytes(SECRET_SHA512), UtcTime::epoch() + 1111111109_s, 8), "25091201"s);

    // T = 00000000023523ED, 2005-03-18 01:58:31
    expectEq$(totp<Sha1>(bytes(SECRET_SHA1), UtcTime::epoch() + 1111111111_s, 8), "14050471"s);
    expectEq$(totp<Sha256>(bytes(SECRET_SHA256), UtcTime::epoch() + 1111111111_s, 8), "67062674"s);
    expectEq$(totp<Sha512>(bytes(SECRET_SHA512), UtcTime::epoch() + 1111111111_s, 8), "99943326"s);

    // T = 000000000273EF07, 2009-02-13 23:31:30
    expectEq$(totp<Sha1>(bytes(SECRET_SHA1), UtcTime::epoch() + 1234567890_s, 8), "89005924"s);
    expectEq$(totp<Sha256>(bytes(SECRET_SHA256), UtcTime::epoch() + 1234567890_s, 8), "91819424"s);
    expectEq$(totp<Sha512>(bytes(SECRET_SHA512), UtcTime::epoch() + 1234567890_s, 8), "93441116"s);

    // T = 0000000003F940AA, 2033-05-18 03:33:20
    expectEq$(totp<Sha1>(bytes(SECRET_SHA1), UtcTime::epoch() + 2000000000_s, 8), "69279037"s);
    expectEq$(totp<Sha256>(bytes(SECRET_SHA256), UtcTime::epoch() + 2000000000_s, 8), "90698825"s);
    expectEq$(totp<Sha512>(bytes(SECRET_SHA512), UtcTime::epoch() + 2000000000_s, 8), "38618901"s);

    // T = 0000000027BC86AA, 2603-10-11 11:33:20
    expectEq$(totp<Sha1>(bytes(SECRET_SHA1), UtcTime::epoch() + 20000000000_s, 8), "65353130"s);
    expectEq$(totp<Sha256>(bytes(SECRET_SHA256), UtcTime::epoch() + 20000000000_s, 8), "77737706"s);
    expectEq$(totp<Sha512>(bytes(SECRET_SHA512), UtcTime::epoch() + 20000000000_s, 8), "47863826"s);

    return Ok();
}

} // namespace Karm::Crypto::Tests
