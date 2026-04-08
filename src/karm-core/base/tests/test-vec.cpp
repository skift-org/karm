import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("vec-default-constructed") {
    Vec<int> vec;

    expectEq$(vec.len(), 0uz);
    expectEq$(vec.cap(), 0uz);
    expectEq$(vec.buf(), nullptr);

    return Ok();
}

test$("vec-push-front-slice") {
    Vec<int> vec = {4, 5};
    Array els{1, 2, 3};
    vec.pushFront(els);

    expectEq$(vec.len(), 5uz);
    expectEq$(vec[0], 1);
    expectEq$(vec[1], 2);
    expectEq$(vec[2], 3);
    expectEq$(vec[3], 4);
    expectEq$(vec[4], 5);

    return Ok();
}

test$("small-vec-inline-storage") {
    SmallVec<int, 4> vec;

    expectEq$(vec.len(), 0uz);
    expectEq$(vec.cap(), 4uz);
    expectNe$(vec.buf(), nullptr);

    vec.pushBack(1);
    vec.pushBack(2);
    vec.pushBack(3);
    vec.pushBack(4);

    expectEq$(vec.len(), 4uz);
    expectEq$(vec.cap(), 4uz);
    expectEq$(vec[0], 1);
    expectEq$(vec[1], 2);
    expectEq$(vec[2], 3);
    expectEq$(vec[3], 4);

    return Ok();
}

test$("small-vec-spills-past-inline-capacity") {
    SmallVec<int, 4> vec = {1, 2, 3, 4};
    auto* beforeSpill = vec.buf();

    vec.pushBack(5);

    expectEq$(vec.len(), 5uz);
    expect$(vec.cap() > 4uz);
    expectNe$(vec.buf(), beforeSpill);
    expectEq$(vec[0], 1);
    expectEq$(vec[1], 2);
    expectEq$(vec[2], 3);
    expectEq$(vec[3], 4);
    expectEq$(vec[4], 5);

    return Ok();
}

test$("small-vec-large-initializer-spills") {
    SmallVec<int, 2> vec = {1, 2, 3};

    expectEq$(vec.len(), 3uz);
    expect$(vec.cap() > 2uz);
    expectEq$(vec[0], 1);
    expectEq$(vec[1], 2);
    expectEq$(vec[2], 3);

    return Ok();
}

test$("vec-niche") {
    Opt<Vec<int>> test;

    auto comp = Vec<int>{5, 0, 2};

    expectEq$(sizeof(test), sizeof(Vec<int>));
    expectEq$(test.has(), false);
    expectEq$(test, NONE);
    test = Vec<int>{5, 0, 2};
    expectEq$(test.unwrap(), comp);
    expectEq$(test.take(), comp);
    expectEq$(test, NONE);
    test = Vec<int>{};
    expectEq$(test.has(), true);

    return Ok();
}

} // namespace Karm::Base::Tests
