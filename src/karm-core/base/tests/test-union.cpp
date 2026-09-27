import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("union-niche") {
    Opt<Union<float, int, double>> test;

    assertEq$(sizeof(test), sizeof(Union<float, int, double>));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(2);
    assertEq$(test.expect(), 2);
    assertEq$(test.take(), 2);
    assertEq$(test, NONE);
    test = Some(1.0f);
    assertEq$(test.has(), true);

    return Ok();
}

} // namespace Karm::Base::Tests
