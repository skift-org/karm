import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("vec-default-constructed") {
    Vec<int> vec;

    assertEq$(vec.len(), 0uz);
    assertEq$(vec.cap(), 0uz);
    assertEq$(vec.buf(), nullptr);

    return Ok();
}

test$("vec-push-front-slice") {
    Vec<int> vec = {4, 5};
    Array els{1, 2, 3};
    vec.pushFront(els);

    assertEq$(vec.len(), 5uz);
    assertEq$(vec[0], 1);
    assertEq$(vec[1], 2);
    assertEq$(vec[2], 3);
    assertEq$(vec[3], 4);
    assertEq$(vec[4], 5);

    return Ok();
}

test$("small-vec-inline-storage") {
    SmallVec<int, 4> vec;

    assertEq$(vec.len(), 0uz);
    assertEq$(vec.cap(), 4uz);
    assertNe$(vec.buf(), nullptr);

    vec.pushBack(1);
    vec.pushBack(2);
    vec.pushBack(3);
    vec.pushBack(4);

    assertEq$(vec.len(), 4uz);
    assertEq$(vec.cap(), 4uz);
    assertEq$(vec[0], 1);
    assertEq$(vec[1], 2);
    assertEq$(vec[2], 3);
    assertEq$(vec[3], 4);

    return Ok();
}

test$("small-vec-spills-past-inline-capacity") {
    SmallVec<int, 4> vec = {1, 2, 3, 4};
    auto* beforeSpill = vec.buf();

    vec.pushBack(5);

    assertEq$(vec.len(), 5uz);
    assert$(vec.cap() > 4uz);
    assertNe$(vec.buf(), beforeSpill);
    assertEq$(vec[0], 1);
    assertEq$(vec[1], 2);
    assertEq$(vec[2], 3);
    assertEq$(vec[3], 4);
    assertEq$(vec[4], 5);

    return Ok();
}

test$("small-vec-large-initializer-spills") {
    SmallVec<int, 2> vec = {1, 2, 3};

    assertEq$(vec.len(), 3uz);
    assert$(vec.cap() > 2uz);
    assertEq$(vec[0], 1);
    assertEq$(vec[1], 2);
    assertEq$(vec[2], 3);

    return Ok();
}

test$("vec-niche") {
    Opt<Vec<int>> test;

    auto comp = Vec<int>{5, 0, 2};

    assertEq$(sizeof(test), sizeof(Vec<int>));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(Vec<int>{5, 0, 2});
    assertEq$(test.expect(), comp);
    assertEq$(test.take(), comp);
    assertEq$(test, NONE);
    test = Some(Vec<int>{});
    assertEq$(test.has(), true);

    return Ok();
}

} // namespace Karm::Base::Tests
