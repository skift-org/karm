#include <karm/test>

import Karm.Core;

namespace Karm::Base::Tests {

test$("range-iter") {
    auto r = irange::zeroTo(5);

    expectEq$(r.next(), 0);
    expectEq$(r.next(), 1);
    expectEq$(r.next(), 2);
    expectEq$(r.next(), 3);
    expectEq$(r.next(), 4);

    return Ok();
}

test$("range-iter-rev") {
    auto r = irange::zeroTo(5).iterRev();

    expectEq$(r.next(), 4);
    expectEq$(r.next(), 3);
    expectEq$(r.next(), 2);
    expectEq$(r.next(), 1);
    expectEq$(r.next(), 0);

    return Ok();
}

} // namespace Karm::Base::Tests
