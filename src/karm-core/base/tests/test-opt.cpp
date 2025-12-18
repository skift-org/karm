#include <karm-test/macros.h>

import Karm.Core;

namespace Karm::Base::Tests {

test$("opt-default-constructor") {
    Opt<int> opt{};

    expect$(not opt.has());

    return Ok();
}

test$("opt-constructed") {
    Opt<int> opt{420};

    expect$(opt.has());
    expectEq$(opt.unwrap(), 420);

    return Ok();
}

test$("opt-assign") {
    Opt<int> opt{};

    opt = 420;

    expect$(opt.has());
    expectEq$(opt.unwrap(), 420);

    return Ok();
}

test$("opt-assign-none") {
    Opt<int> opt{420};

    opt = NONE;

    expect$(not opt.has());

    return Ok();
}

test$("opt-unwrap") {
    Opt<int> opt{420};

    expectEq$(opt.unwrap(), 420);

    return Ok();
}

test$("opt-take") {
    Opt<int> opt{420};

    expectEq$(opt.take(), 420);
    expect$(not opt.has());

    return Ok();
}

test$("opt-equal") {
    Opt<int> opt = NONE;
    expectEq$(opt, NONE);
    expectNe$(opt, 42);

    opt = 42;
    expectEq$(opt, 42);
    expectNe$(opt, NONE);

    return Ok();
}

test$("bool-niche") {
    Opt<bool> test;

    expectEq$(sizeof(test), sizeof(bool));
    expectEq$(test.has(), false);
    expectEq$(test, NONE);
    test = true;
    expectEq$(test.unwrap(), true);
    expectEq$(test.take(), true);
    expectEq$(test, NONE);
    test = false;
    expectEq$(test.has(), true);
    test = 2;
    expectEq$(test.has(), true);

    return Ok();
}

enum struct TestEnum {
    A,
    B,

    _LEN,
};

test$("bool-niche") {
    Opt<TestEnum> test;

    expectEq$(sizeof(test), sizeof(TestEnum));
    expectEq$(test.has(), false);
    expectEq$(test, NONE);
    test = TestEnum::A;
    expectEq$(test.unwrap(), TestEnum::A);
    expectEq$(test.take(), TestEnum::A);
    expectEq$(test, NONE);
    test = TestEnum::_LEN;
    expectEq$(test.has(), true);

    return Ok();
}

test$("opt-ref-default-constructor") {
    Opt<int&> opt{};

    expect$(not opt.has());
    expectEq$(opt, NONE);

    return Ok();
}

test$("opt-ref-constructed") {
    int value = 42;
    Opt<int&> opt{value};

    expect$(opt.has());
    expectEq$(opt.unwrap(), 42);
    expectEq$(&opt.unwrap(), &value); // really is a reference to value

    return Ok();
}

test$("opt-ref-assign") {
    int a = 1;
    int b = 2;

    Opt<int&> opt{a};
    expect$(opt.has());
    expectEq$(&opt.unwrap(), &a);
    expectEq$(opt.unwrap(), 1);

    opt = b;
    expect$(opt.has());
    expectEq$(&opt.unwrap(), &b);
    expectEq$(opt.unwrap(), 2);

    // Mutating via the opt mutates the underlying object.
    opt.unwrap() = 10;
    expectEq$(b, 10);

    return Ok();
}

test$("opt-ref-assign-none") {
    int value = 123;
    Opt<int&> opt{value};

    expect$(opt.has());

    opt = NONE;

    expect$(not opt.has());
    expectEq$(opt, NONE);

    return Ok();
}

test$("opt-ref-unwrap") {
    int value = 7;
    Opt<int&> opt{value};

    expectEq$(opt.unwrap(), 7);
    expectEq$(&opt.unwrap(), &value);

    // Changing the original is visible through the Opt.
    value = 9;
    expectEq$(opt.unwrap(), 9);

    return Ok();
}

test$("opt-ref-take") {
    int value = 123;
    Opt<int&> opt{value};

    int& ref = opt.take();

    // still refers to the same object
    expectEq$(&ref, &value);
    expect$(not opt.has());

    // take() should not destroy, only unbind
    ref = 321;
    expectEq$(value, 321);

    return Ok();
}

test$("opt-const-ref") {
    int value = 5;
    Opt<int const&> opt{value};

    expect$(opt.has());
    expectEq$(opt.unwrap(), 5);

    // Aliasing semantics: changes in the original are seen through the const ref.
    value = 8;
    expectEq$(opt.unwrap(), 8);

    opt = NONE;
    expect$(not opt.has());

    return Ok();
}

test$("opt-ref-operator-bool-and-clear") {
    int value = 1;
    Opt<int&> opt{};

    expect$(not opt);
    expect$(not opt.has());

    opt = value;
    expect$(opt);
    expect$(opt.has());

    opt.clear();
    expect$(not opt);
    expect$(not opt.has());

    return Ok();
}

} // namespace Karm::Base::Tests
