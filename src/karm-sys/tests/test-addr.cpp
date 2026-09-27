#include <karm/test>

import Karm.Sys;

namespace Karm::Sys::Tests {

test$("ip4-eq") {
    assertEq$(Ip4::localhost(), Ip4::localhost());
    assertEq$(Ip4::localhost(80), Ip4::localhost(80));
    return Ok();
}

} // namespace Karm::Sys::Tests
