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

// MARK: Boxed arrays ---------------------------------------------------------

test$("box-array-filled") {
    auto box = Box<int[]>::filled(3, 7);

    assertEq$(box.len(), 3uz);
    for (auto v : box)
        assertEq$(v, 7);

    box[1] = 42;
    assertEq$(box[1], 42);

    return Ok();
}

test$("box-array-empty") {
    Box<int[]> box;

    assertEq$(box.len(), 0uz);
    assertEq$(box.buf(), nullptr);

    return Ok();
}

test$("box-array-copy") {
    Array<int, 3> values = {1, 2, 3};
    Box<int[]> box = values;
    assertEq$(sub(box), sub(values));

    Box<int[]> copy = box;
    copy[0] = 42;
    assertEq$(box[0], 1);
    assertEq$(copy[0], 42);

    return Ok();
}

test$("box-array-move") {
    auto box = Box<int[]>::filled(3, 7);
    auto* buf = box.buf();

    Box<int[]> moved = std::move(box);
    assertEq$(moved.buf(), buf);
    assertEq$(moved.len(), 3uz);
    assertEq$(box.len(), 0uz);
    assertEq$(box.buf(), nullptr);

    return Ok();
}

test$("box-array-niche") {
    Opt<Box<int[]>> test;

    assertEq$(sizeof(test), sizeof(Box<int[]>));
    assertEq$(test.has(), false);
    test = Some(Box<int[]>::filled(2, 1));
    assertEq$(test.has(), true);

    return Ok();
}

struct NotDefaultConstructible {
    int value;

    NotDefaultConstructible() = delete;

    NotDefaultConstructible(int value) : value(value) {}
};

test$("box-array-not-default-constructible") {
    Array<NotDefaultConstructible, 2> values = {NotDefaultConstructible{1}, NotDefaultConstructible{2}};
    Box<NotDefaultConstructible[]> box = values;

    assertEq$(box.len(), 2uz);
    assertEq$(box[1].value, 2);

    return Ok();
}

test$("box-array-vec-roundtrip") {
    auto vec = Vec<int>::filled(3, 7);
    auto* buf = vec.buf();

    // NOTE: The capacity already matches the length, so neither
    //       conversion should reallocate or copy the elements.
    Box<int[]> box = vec.boxed();
    assertEq$(box.buf(), buf);
    assertEq$(box.len(), 3uz);
    assertEq$(vec.len(), 0uz);

    Vec<int> back = std::move(box);
    assertEq$(back.buf(), buf);
    assertEq$(back.len(), 3uz);
    assertEq$(box.len(), 0uz);

    return Ok();
}

test$("box-array-vec-shrinks") {
    Vec<int> vec;
    vec.ensure(16);
    vec.pushBack(1);
    vec.pushBack(2);

    Box<int[]> box = vec.boxed();
    assertEq$(box.len(), 2uz);
    assertEq$(box[0], 1);
    assertEq$(box[1], 2);

    return Ok();
}

} // namespace Karm::Base::Tests
