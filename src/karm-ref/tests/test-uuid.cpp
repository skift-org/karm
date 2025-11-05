#include <karm-test/macros.h>

import Karm.Ref;

namespace Karm::Ref::Tests {

test$("karm-ref-uuid-parse") {
    auto uuid = try$(Uuid::v4());
    auto uuidStr = Io::format("{}", uuid);
    auto uuid2 = try$(Uuid::parse(uuidStr));
    auto uuid2Str = Io::format("{}", uuid2);

    expectEq$(uuidStr, uuid2Str);
    expectEq$(uuid, uuid2);

    return Ok();
}

} // namespace Karm::Ref::Tests
