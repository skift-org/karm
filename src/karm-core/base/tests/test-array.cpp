#include <karm/test>

import Karm.Core;

namespace Karm::Base::Tests {

test$("array-niche") {
    using Test = Array<bool, 2>;
    Test value = Test{true, false};
    Opt<Test> test;

    assertEq$(sizeof(test), sizeof(Test));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(Test{true, false});
    assertEq$(test.expect(), value);
    assertEq$(test.take(), value);
    assertEq$(test, NONE);
    test = Some(Test{0, 0});
    assertEq$(test.has(), true);

    return Ok();
}

} // namespace Karm::Base::Tests
