import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("box-niche") {
    Opt<Box<int>> test;

    assertEq$(sizeof(test), sizeof(Box<int>));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(makeBox<int>(5));
    assertEq$(test.expect(), 5);
    assertEq$(test.take(), 5);
    assertEq$(test, NONE);
    test = Some(makeBox<int>());
    assertEq$(test.has(), true);

    return Ok();
}

struct TestType {
    bool deleted = false;
};

struct TestDeleter {
    void operator()(TestType* p) const {
        p->deleted = true;
    };
};

test$("box-deleter") {
    TestType test;
    {
        Box<TestType, TestDeleter> testBox(MOVE, &test);
    }
    assert$(test.deleted);

    return Ok();
}

} // namespace Karm::Base::Tests
