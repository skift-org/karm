import Karm.Core;
#include <karm/test>

namespace Karm::Base::Tests {

test$("map-put") {
    Map<int, int> map{};
    map.put(420, 69);

    expectEq$(map.len(), 1uz);
    expect$(map.contains(420));
    expect$(not map.contains(69));

    return Ok();
}

test$("map-put-update") {
    Map<int, int> map{};
    map.put(1, 100);
    expectEq$(map.len(), 1uz);

    map.put(1, 200);
    expectEq$(map.len(), 1uz);

    auto val = map.lookup(1);
    expect$(static_cast<bool>(val));
    expectEq$(val.unwrap(), 200);

    return Ok();
}

test$("map-remove") {
    Map<int, int> map{};
    map.put(1, 100);
    expect$(map.contains(1));

    expect$(map.remove(1));
    expect$(not map.contains(1));
    expectEq$(map.len(), 0uz);

    expect$(not map.remove(999));

    return Ok();
}

test$("map-clear") {
    Map<int, int> map{};
    map.put(1, 10);
    map.put(2, 20);

    expectEq$(map.len(), 2uz);
    map.clear();

    expectEq$(map.len(), 0uz);
    expect$(not map.contains(1));
    expect$(not map.contains(2));

    return Ok();
}

test$("map-init-list") {
    Map<int, int> map{
        {1, 10},
        {2, 20},
        {3, 30}
    };

    expectEq$(map.len(), 3uz);
    expect$(map.contains(1));
    expect$(map.contains(3));
    expect$(not map.contains(4));

    return Ok();
}

test$("map-lookup") {
    Map<int, int> map{{1, 100}};

    auto found = map.lookup(1);
    expect$(static_cast<bool>(found));

    auto notFound = map.lookup(2);
    expect$(not static_cast<bool>(notFound));

    return Ok();
}

test$("map-iter-keys") {
    Map<int, int> map{{1, 10}, {2, 20}, {3, 30}};
    usize count = 0;

    for (auto const& key : map.iter()) {
        expect$(map.contains(key));
        count++;
    }

    expectEq$(count, 3uz);

    return Ok();
}

test$("map-iter-items") {
    Map<int, int> map{{1, 10}, {2, 20}};
    usize count = 0;

    for (auto const& item : map.iterItems()) {
        expect$(map.contains(item.key));
        auto val = map.lookup(item.key);
        expect$(static_cast<bool>(val));
        expectEq$(val.unwrap(), item.value);
        count++;
    }
    expectEq$(count, 2uz);

    return Ok();
}

test$("map-bool-operator") {
    Map<int, int> map{};
    expect$(not static_cast<bool>(map));

    map.put(1, 10);
    expect$(static_cast<bool>(map));

    return Ok();
}

test$("map-eq-operator") {
    Map<int, int> m1{{1, 10}, {2, 20}};
    Map<int, int> m2{{2, 20}, {1, 10}};
    Map<int, int> m3{{1, 10}};
    Map<int, int> m4{{1, 99}, {2, 20}};

    expect$(m1 == m2);
    expect$(not(m1 == m3));
    expect$(not(m1 == m4));

    return Ok();
}

test$("map-ensure") {
    Map<int, int> map{};
    map.ensure(50);

    expectEq$(map.len(), 0uz);
    map.put(1, 10);
    expectEq$(map.len(), 1uz);

    return Ok();
}

} // namespace Karm::Base::Tests