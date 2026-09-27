import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

enum struct Option : u8 {
    FOO = 1 << 0,
    BAR = 1 << 1,
    BAZ = 1 << 2,
};

test$("flags-empty") {
    Flags<Option> flags;

    assertEq$(flags.empty(), true);
    assertEq$(flags.any(), false);
    assertEq$(flags.raw(), 0);

    assertEq$(flags.has(Option::FOO), false);
    assertEq$(flags.has(Option::BAR), false);
    assertEq$(flags.has(Option::BAZ), false);

    return Ok();
}

test$("flags-all") {
    Flags<Option> flags{Option::FOO, Option::BAR, Option::BAZ};

    assertEq$(flags.empty(), false);
    assertEq$(flags.any(), true);
    assertEq$(flags.raw(), 0b111);

    assertEq$(flags.has(Option::FOO), true);
    assertEq$(flags.has(Option::BAR), true);
    assertEq$(flags.has(Option::BAZ), true);

    return Ok();
}

test$("flags-union") {
    Flags<Option> a = {Option::FOO};
    Flags<Option> b = {Option::BAZ};
    Flags<Option> c = a | b;

    assertEq$(c.empty(), false);
    assertEq$(c.any(), true);
    assertEq$(c.raw(), 0b101);

    assertEq$(c.has(Option::FOO), true);
    assertEq$(c.has(Option::BAR), false);
    assertEq$(c.has(Option::BAZ), true);

    return Ok();
}

test$("flags-clear") {
    Flags<Option> flags{Option::FOO, Option::BAR, Option::BAZ};

    assertEq$(flags.empty(), false);
    assertEq$(flags.any(), true);
    assertEq$(flags.raw(), 0b111);

    flags.clear();

    assertEq$(flags.empty(), true);
    assertEq$(flags.any(), false);
    assertEq$(flags.raw(), 0);

    return Ok();
}

} // namespace Karm::Base::Tests
