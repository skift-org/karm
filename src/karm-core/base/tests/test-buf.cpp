import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("buf-niche") {
    Opt<Buf<int>> test;

    auto comp = Buf<int>::init(5, 0);

    assertEq$(sizeof(test), sizeof(Buf<int>));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(Buf<int>::init(5, 0));
    assertEq$(test.expect(), comp);
    assertEq$(test.take(), comp);
    assertEq$(test, NONE);
    test = Some(Buf<int>::init(0));
    assertEq$(test.has(), true);

    return Ok();
}

} // namespace Karm::Base::Tests
