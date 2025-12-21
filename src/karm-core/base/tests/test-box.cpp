import Karm.Core;

#include <karm-test/macros.h>

namespace Karm::Base::Tests {

test$("box-niche") {
    Opt<Box<int>> test;

    expectEq$(sizeof(test), sizeof(Box<int>));
    expectEq$(test.has(), false);
    expectEq$(test, NONE);
    test = makeBox<int>(5);
    expectEq$(test.unwrap(), 5);
    expectEq$(test.take(), 5);
    expectEq$(test, NONE);
    test = makeBox<int>();
    expectEq$(test.has(), true);

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
    expect$(test.deleted);

    return Ok();
}

} // namespace Karm::Base::Tests
