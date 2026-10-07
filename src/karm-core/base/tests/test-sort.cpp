import Karm.Core;

#include <karm/test>

namespace Karm::Base {

struct Foo {
    isize value;
    isize order;

    auto operator<=>(Foo const& other) const {
        return order <=> other.order;
    }
};

test$("sort-basic") {
    Array arr{
        Foo{1, 1},
        Foo{2, 2},
        Foo{3, 3},
        Foo{4, 4},
        Foo{5, 5},
    };

    sort(arr);

    assertEq$(arr[0].value, 1);
    assertEq$(arr[1].value, 2);
    assertEq$(arr[2].value, 3);
    assertEq$(arr[3].value, 4);
    assertEq$(arr[4].value, 5);

    return Ok();
}

test$("stable-sort") {
    Array arr{
        Foo{1, 1},
        Foo{2, 2},
        Foo{3, 3},
        Foo{4, 2},
        Foo{5, 1},
    };

    stableSort(arr);

    assertEq$(arr[0].value, 1);
    assertEq$(arr[1].value, 5);
    assertEq$(arr[2].value, 2);
    assertEq$(arr[3].value, 4);
    assertEq$(arr[4].value, 3);

    return Ok();
}

test$("stable-sort-small") {
    Array arr{
        2,
        1
    };

    stableSort(arr);

    assertEq$(arr[0], 1);
    assertEq$(arr[1], 2);

    return Ok();
}

} // namespace Karm::Base
