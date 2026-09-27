import Karm.Core;
#include <karm/test>

namespace Karm::Base::Tests {

test$("map-put") {
    Map<int, int> map{};
    map.put(420, 69);

    assertEq$(map.len(), 1uz);
    assert$(map.contains(420));
    assert$(not map.contains(69));

    return Ok();
}

test$("map-put-update") {
    Map<int, int> map{};
    map.put(1, 100);
    assertEq$(map.len(), 1uz);

    map.put(1, 200);
    assertEq$(map.len(), 1uz);

    auto val = map.lookup(1);
    assert$(static_cast<bool>(val));
    assertEq$(val.expect(), 200);

    return Ok();
}

test$("map-remove") {
    Map<int, int> map{};
    map.put(1, 100);
    assert$(map.contains(1));

    assert$(static_cast<bool>(map.remove(1)));
    assert$(not map.contains(1));
    assertEq$(map.len(), 0uz);

    assert$(not static_cast<bool>(map.remove(999)));

    return Ok();
}

test$("map-clear") {
    Map<int, int> map{};
    map.put(1, 10);
    map.put(2, 20);

    assertEq$(map.len(), 2uz);
    map.clear();

    assertEq$(map.len(), 0uz);
    assert$(not map.contains(1));
    assert$(not map.contains(2));

    return Ok();
}

test$("map-init-list") {
    Map<int, int> map{
        {1, 10},
        {2, 20},
        {3, 30}
    };

    assertEq$(map.len(), 3uz);
    assert$(map.contains(1));
    assert$(map.contains(3));
    assert$(not map.contains(4));

    return Ok();
}

test$("map-lookup") {
    Map<int, int> map{{1, 100}};

    auto found = map.lookup(1);
    assert$(static_cast<bool>(found));

    auto notFound = map.lookup(2);
    assert$(not static_cast<bool>(notFound));

    return Ok();
}

test$("map-iter-keys") {
    Map<int, int> map{{1, 10}, {2, 20}, {3, 30}};
    usize count = 0;

    for (auto const& key : map.iter()) {
        assert$(map.contains(key));
        count++;
    }

    assertEq$(count, 3uz);

    return Ok();
}

test$("map-iter-items") {
    Map<int, int> map{{1, 10}, {2, 20}};
    usize count = 0;

    for (auto const& item : map.iterItems()) {
        assert$(map.contains(item.key));
        auto val = map.lookup(item.key);
        assert$(static_cast<bool>(val));
        assertEq$(val.expect(), item.value);
        count++;
    }
    assertEq$(count, 2uz);

    return Ok();
}

test$("map-bool-operator") {
    Map<int, int> map{};
    assert$(not static_cast<bool>(map));

    map.put(1, 10);
    assert$(static_cast<bool>(map));

    return Ok();
}

test$("map-eq-operator") {
    Map<int, int> m1{{1, 10}, {2, 20}};
    Map<int, int> m2{{2, 20}, {1, 10}};
    Map<int, int> m3{{1, 10}};
    Map<int, int> m4{{1, 99}, {2, 20}};

    assert$(m1 == m2);
    assert$(not(m1 == m3));
    assert$(not(m1 == m4));

    return Ok();
}

test$("map-ensure") {
    Map<int, int> map{};
    map.ensure(50);

    assertEq$(map.len(), 0uz);
    map.put(1, 10);
    assertEq$(map.len(), 1uz);

    return Ok();
}

} // namespace Karm::Base::Tests
