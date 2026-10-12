import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("align-down") {
    assertEq$(alignDown(0, 8), 0uz);
    assertEq$(alignDown(7, 8), 0uz);
    assertEq$(alignDown(8, 8), 8uz);
    assertEq$(alignDown(9, 8), 8uz);

    // Non power of two alignments
    assertEq$(alignDown(23, 24), 0uz);
    assertEq$(alignDown(24, 24), 24uz);
    assertEq$(alignDown(25, 24), 24uz);

    return Ok();
}

test$("align-up") {
    assertEq$(alignUp(0, 8), 0uz);
    assertEq$(alignUp(7, 8), 8uz);
    assertEq$(alignUp(8, 8), 8uz);
    assertEq$(alignUp(9, 8), 16uz);

    // Non power of two alignments
    assertEq$(alignUp(1, 24), 24uz);
    assertEq$(alignUp(24, 24), 24uz);
    assertEq$(alignUp(25, 24), 48uz);

    return Ok();
}

test$("align-aligned") {
    assertEq$(aligned(0, 8), true);
    assertEq$(aligned(16, 8), true);
    assertEq$(aligned(17, 8), false);

    assertEq$(aligned(48, 24), true);
    assertEq$(aligned(32, 24), false);

    return Ok();
}

test$("align-ceil-div") {
    assertEq$(ceilDiv(0, 8), 0uz);
    assertEq$(ceilDiv(1, 8), 1uz);
    assertEq$(ceilDiv(8, 8), 1uz);
    assertEq$(ceilDiv(9, 8), 2uz);
    assertEq$(ceilDiv(396, 16), 25uz);

    return Ok();
}

} // namespace Karm::Base::Tests
