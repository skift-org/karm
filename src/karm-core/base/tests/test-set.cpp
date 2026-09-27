import Karm.Core;
#include <karm/test>

namespace Karm::Base::Tests {

test$("set-put") {
    Set<int> set{};
    set.add(420);
    assertEq$(set.len(), 1uz);
    assert$(set.contains(420));

    return Ok();
}

test$("set-remove") {
    Set<int> set{};
    set.add(420);
    assert$(set.contains(420));
    set.remove(420);
    assert$(not set.contains(420));

    return Ok();
}

test$("set-clear") {
    Set<int> set{};
    set.add(420);
    set.add(69);
    assert$(set.contains(420));
    assert$(set.contains(69));

    set.clear();
    assert$(not set.contains(420));
    assert$(not set.contains(69));

    return Ok();
}

test$("set-len") {
    Set<int> set{};
    assertEq$(set.len(), 0uz);
    set.add(420);
    assertEq$(set.len(), 1uz);
    set.add(69);
    assertEq$(set.len(), 2uz);
    set.remove(420);
    assertEq$(set.len(), 1uz);

    return Ok();
}

test$("set-put-collision") {
    Set<int> set;
    set.add(420);
    set.add(69);
    set.add(420);
    assertEq$(set.len(), 2uz);

    return Ok();
}

test$("set-put-resize") {
    Set<int> set;
    for (int i = 0; i < 10; i++) {
        set.add(i);
    }
    assertEq$(set.len(), 10uz);
    set.add(420);
    assert$(set.contains(420));
    return Ok();
}

test$("set-init-list") {
    Set<int> set{1, 2, 3, 4, 4, 5};
    assertEq$(set.len(), 5uz);
    assert$(set.contains(1));
    assert$(set.contains(5));
    assert$(not set.contains(6));

    return Ok();
}

test$("set-ensure") {
    Set<int> set{};
    set.ensure(100);
    assertEq$(set.len(), 0uz);
    set.add(420);
    assertEq$(set.len(), 1uz);

    return Ok();
}

test$("set-add-from") {
    Set<int> source{1, 2, 3};
    Set<int> dest{3, 4, 5};

    dest.addFrom(source);

    assertEq$(dest.len(), 5uz);
    assert$(dest.contains(1));
    assert$(dest.contains(3));
    assert$(dest.contains(5));

    return Ok();
}

test$("set-lookup") {
    Set<int> set{420, 69};

    auto found = set.lookup(420);
    assert$(static_cast<bool>(found));
    assertEq$(found.expect(), 420);

    auto notFound = set.lookup(999);
    assert$(not static_cast<bool>(notFound));

    return Ok();
}

test$("set-iter") {
    Set<int> set{1, 2, 3};
    usize count = 0;

    for (auto const& val : set.iter()) {
        assert$(set.contains(val));
        count++;
    }

    assertEq$(count, 3uz);

    return Ok();
}

test$("set-bool-operator") {
    Set<int> set{};
    assert$(not static_cast<bool>(set));

    set.add(1);
    assert$(static_cast<bool>(set));

    return Ok();
}

test$("set-eq-operator") {
    Set<int> set1{1, 2, 3};
    Set<int> set2{3, 2, 1}; // Order shouldn't matter
    Set<int> set3{1, 2};

    assert$(set1 == set2);
    assert$(not(set1 == set3));

    return Ok();
}

test$("set-union-operator") {
    Set<int> a{1, 2, 3};
    Set<int> b{3, 4, 5};

    auto res = a | b;

    assertEq$(res.len(), 5uz);
    assert$(res.contains(1));
    assert$(res.contains(3));
    assert$(res.contains(5));

    return Ok();
}

test$("set-intersection-operator") {
    Set<int> a{1, 2, 3, 4};
    Set<int> b{3, 4, 5, 6};

    auto res = a & b;

    assertEq$(res.len(), 2uz);
    assert$(res.contains(3));
    assert$(res.contains(4));
    assert$(not res.contains(1));
    assert$(not res.contains(5));

    return Ok();
}

test$("set-difference-operator") {
    Set<int> a{1, 2, 3, 4};
    Set<int> b{3, 4, 5};

    auto res = a - b;
    assertEq$(res.len(), 2uz);
    assert$(res.contains(1));
    assert$(res.contains(2));
    assert$(not res.contains(3));

    return Ok();
}

test$("set-symmetric-difference-operator") {
    Set<int> a{1, 2, 3};
    Set<int> b{3, 4, 5};

    auto res = a ^ b;

    assertEq$(res.len(), 4uz);
    assert$(res.contains(1));
    assert$(res.contains(2));
    assert$(res.contains(4));
    assert$(res.contains(5));
    assert$(not res.contains(3));

    return Ok();
}

} // namespace Karm::Base::Tests