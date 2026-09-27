#include <karm/test>

import Karm.Core;

namespace Karm::Math::Tests {

test$("floori") {
    assertEq$(0, floori(0.0));
    assertEq$(0, floori(0.1));
    assertEq$(0, floori(0.5));
    assertEq$(0, floori(0.9));
    assertEq$(1, floori(1.0));
    assertEq$(-1, floori(-0.1));
    assertEq$(-1, floori(-0.5));
    assertEq$(-1, floori(-0.9));

    return Ok();
}

test$("ceili") {
    assertEq$(0, ceili(0.0));
    assertEq$(1, ceili(0.1));
    assertEq$(1, ceili(0.5));
    assertEq$(1, ceili(0.9));
    assertEq$(1, ceili(1.0));
    assertEq$(0, ceili(-0.1));
    assertEq$(0, ceili(-0.5));
    assertEq$(0, ceili(-0.9));

    return Ok();
}

test$("roundi") {
    assertEq$(0, roundi(0.0));

    assertEq$(0, roundi(0.1));
    assertEq$(1, roundi(0.5));
    assertEq$(1, roundi(0.9));
    assertEq$(1, roundi(1.0));

    assertEq$(0, roundi(-0.1));
    assertEq$(-1, roundi(-0.5));
    assertEq$(-1, roundi(-0.9));
    assertEq$(-1, roundi(-1.0));

    return Ok();
}

} // namespace Karm::Math::Tests
