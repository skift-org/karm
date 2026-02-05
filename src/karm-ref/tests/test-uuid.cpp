#include <karm/test>

import Karm.Ref;

namespace Karm::Ref::Tests {

test$("karm-ref-uuid-parse") {
    auto uuid = "5fb3610d-8cea-4fec-864e-58e9f9c9814d"_uuid;
    auto uuidStr = Io::format("{}", uuid);
    auto uuid2 = try$(Uuid::parse(uuidStr));
    auto uuid2Str = Io::format("{}", uuid2);

    expectEq$(uuidStr, uuid2Str);
    expectEq$(uuid, uuid2);

    return Ok();
}

test$("karm-ref-guid-parse") {
    auto guid = "5fb3610d-8cea-4fec-864e-58e9f9c9814d"_guid;
    auto guidStr = Io::format("{}", guid);
    auto guid2 = try$(Guid::parse(guidStr));
    auto guid2Str = Io::format("{}", guid2);

    expectEq$(guidStr, guid2Str);
    expectEq$(guid, guid2);

    return Ok();
}

test$("karm-ref-uuid-bytes") {
    Array<u8, 16> uuidByteArray = {0x5f,0xb3,0x61,0x0d,0x8c,0xea,0x4f,0xec,0x86,0x4e,0x58,0xe9,0xf9,0xc9,0x81,0x4d};
    auto uuid = "5fb3610d-8cea-4fec-864e-58e9f9c9814d"_uuid;
    expectEq$(uuid.bytes(), bytes(uuidByteArray));
    return Ok();
}

test$("karm-ref-guid-bytes") {
    Array<u8, 16> guidByteArray = {0x0d,0x61,0xb3,0x5f,0xea,0x8c,0xec,0x4f,0x86,0x4e,0x58,0xe9,0xf9,0xc9,0x81,0x4d};
    auto guid = "5fb3610d-8cea-4fec-864e-58e9f9c9814d"_guid;
    expectEq$(guid.bytes(), bytes(guidByteArray));
    return Ok();
}

} // namespace Karm::Ref::Tests
