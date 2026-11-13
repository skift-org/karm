import Karm.Core;

#include <karm/test>

using namespace Karm::Literals;

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

test$("vec-push-back-slice") {
    Vec<int> vec = {1, 2};
    Array els{3, 4, 5};
    vec.pushBack(els);

    Array expected{1, 2, 3, 4, 5};
    assert$(vec == expected);

    return Ok();
}

test$("vec-insert-many") {
    // Fewer elements than what sits after them
    Vec<int> vec = {1, 2, 3, 4, 5};
    Array two{10, 11};
    vec.insertMany(1, two);
    Array expectedTwo{1, 10, 11, 2, 3, 4, 5};
    assert$(vec == expectedTwo);

    // More elements than what sits after them
    vec = {1, 2, 3};
    Array five{10, 11, 12, 13, 14};
    vec.insertMany(2, five);
    Array expectedFive{1, 2, 10, 11, 12, 13, 14, 3};
    assert$(vec == expectedFive);

    vec = {1, 2};
    vec.insertMany(0, two);
    vec.insertMany(vec.len(), two);
    Array expectedEnds{10, 11, 1, 2, 10, 11};
    assert$(vec == expectedEnds);

    vec.insertMany(3, Slice<int>{});
    assert$(vec == expectedEnds);

    Vec<int> empty;
    empty.insertMany(0, two);
    assert$(empty == two);

    return Ok();
}

test$("vec-insert-many-non-trivial") {
    Vec<String> vec = {"a"s, "b"s, "c"s, "d"s};
    Array<String, 3> els{"x"s, "y"s, "z"s};
    vec.insertMany(1, els);

    Array<String, 7> expected{"a"s, "x"s, "y"s, "z"s, "b"s, "c"s, "d"s};
    assert$(vec == expected);

    assertEq$(els[0], "x"s);
    assertEq$(els[2], "z"s);

    return Ok();
}

test$("inline-vec-insert-many") {
    InlineVec<int, 8> vec = {1, 2, 3};
    Array els{10, 11, 12, 13};
    vec.insertMany(1, els);

    Array expected{1, 10, 11, 12, 13, 2, 3};
    assert$(vec == expected);

    return Ok();
}

test$("small-vec-insert-many") {
    SmallVec<int, 4> vec;
    vec.pushBack(1);
    vec.pushBack(2);

    Array one{10};
    vec.insertMany(1, one);
    Array expectedInline{1, 10, 2};
    assert$(vec == expectedInline);

    // Has to spill to the heap
    Array els{20, 21, 22};
    vec.insertMany(2, els);
    Array expectedSpilled{1, 10, 20, 21, 22, 2};
    assert$(vec == expectedSpilled);

    vec.insertMany(1, els);
    Array expectedAgain{1, 20, 21, 22, 10, 20, 21, 22, 2};
    assert$(vec == expectedAgain);

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
