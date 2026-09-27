#include <karm/test>

import Karm.Core;

namespace Karm::Base::Tests {

test$("range-iter") {
    auto r = irange::zeroTo(5);

    assertEq$(r.next(), 0);
    assertEq$(r.next(), 1);
    assertEq$(r.next(), 2);
    assertEq$(r.next(), 3);
    assertEq$(r.next(), 4);

    return Ok();
}

test$("range-iter-rev") {
    auto r = irange::zeroTo(5).iterRev();

    assertEq$(r.next(), 4);
    assertEq$(r.next(), 3);
    assertEq$(r.next(), 2);
    assertEq$(r.next(), 1);
    assertEq$(r.next(), 0);

    return Ok();
}

} // namespace Karm::Base::Tests
