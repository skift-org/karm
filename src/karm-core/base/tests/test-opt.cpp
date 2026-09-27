#include <karm/test>

import Karm.Core;

namespace Karm::Base::Tests {

test$("opt-default-constructor") {
    Opt<int> opt{};

    assert$(not opt.has());

    return Ok();
}

test$("opt-constructed") {
    Opt<int> opt{Some(420)};

    assert$(opt.has());
    assertEq$(opt.expect(), 420);

    return Ok();
}

test$("opt-assign") {
    Opt<int> opt{};

    opt = Some(420);

    assert$(opt.has());
    assertEq$(opt.expect(), 420);

    return Ok();
}

test$("opt-assign-none") {
    Opt<int> opt{Some(420)};

    opt = NONE;

    assert$(not opt.has());

    return Ok();
}

test$("opt-unwrap") {
    Opt<int> opt{Some(420)};

    assertEq$(opt.expect(), 420);

    return Ok();
}

test$("opt-take") {
    Opt<int> opt{Some(420)};

    assertEq$(opt.take(), 420);
    assert$(not opt.has());

    return Ok();
}

test$("opt-equal") {
    Opt<int> opt = NONE;
    assertEq$(opt, NONE);
    assertNe$(opt, 42);

    opt = Some(42);
    assertEq$(opt, 42);
    assertNe$(opt, NONE);

    return Ok();
}

test$("bool-niche") {
    Opt<bool> test;

    assertEq$(sizeof(test), sizeof(bool));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(true);
    assertEq$(test.expect(), true);
    assertEq$(test.take(), true);
    assertEq$(test, NONE);
    test = Some(false);
    assertEq$(test.has(), true);
    test = Some(2);
    assertEq$(test.has(), true);

    return Ok();
}

enum struct TestEnum {
    A,
    B,

    _LEN,
};

test$("bool-niche") {
    Opt<TestEnum> test;

    assertEq$(sizeof(test), sizeof(TestEnum));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(TestEnum::A);
    assertEq$(test.expect(), TestEnum::A);
    assertEq$(test.take(), TestEnum::A);
    assertEq$(test, NONE);
    test = Some(TestEnum::_LEN);
    assertEq$(test.has(), true);

    return Ok();
}

test$("opt-ref-default-constructor") {
    Opt<int&> opt{};

    assert$(not opt.has());
    assertEq$(opt, NONE);

    return Ok();
}

test$("opt-ref-constructed") {
    int value = 42;
    Opt<int&> opt{Some(value)};

    assert$(opt.has());
    assertEq$(opt.expect(), 42);
    assertEq$(&opt.expect(), &value); // really is a reference to value

    return Ok();
}

test$("opt-ref-assign") {
    int a = 1;
    int b = 2;

    Opt<int&> opt{Some(a)};
    assert$(opt.has());
    assertEq$(&opt.expect(), &a);
    assertEq$(opt.expect(), 1);

    opt = Some(b);
    assert$(opt.has());
    assertEq$(&opt.expect(), &b);
    assertEq$(opt.expect(), 2);

    // Mutating via the opt mutates the underlying object.
    opt.expect() = 10;
    assertEq$(b, 10);

    return Ok();
}

test$("opt-ref-assign-none") {
    int value = 123;
    Opt<int&> opt{Some(value)};

    assert$(opt.has());

    opt = NONE;

    assert$(not opt.has());
    assertEq$(opt, NONE);

    return Ok();
}

test$("opt-ref-unwrap") {
    int value = 7;
    Opt<int&> opt{Some(value)};

    assertEq$(opt.expect(), 7);
    assertEq$(&opt.expect(), &value);

    // Changing the original is visible through the Opt.
    value = 9;
    assertEq$(opt.expect(), 9);

    return Ok();
}

test$("opt-ref-take") {
    int value = 123;
    Opt<int&> opt{Some(value)};

    int& ref = opt.take();

    // still refers to the same object
    assertEq$(&ref, &value);
    assert$(not opt.has());

    // take() should not destroy, only unbind
    ref = 321;
    assertEq$(value, 321);

    return Ok();
}

test$("opt-const-ref") {
    int value = 5;
    Opt<int const&> opt{Some(value)};

    assert$(opt.has());
    assertEq$(opt.expect(), 5);

    // Aliasing semantics: changes in the original are seen through the const ref.
    value = 8;
    assertEq$(opt.expect(), 8);

    opt = NONE;
    assert$(not opt.has());

    return Ok();
}

test$("opt-ref-operator-bool-and-clear") {
    int value = 1;
    Opt<int&> opt{};

    assert$(not opt);
    assert$(not opt.has());

    opt = Some(value);
    assert$(opt);
    assert$(opt.has());

    opt.clear();
    assert$(not opt);
    assert$(not opt.has());

    return Ok();
}

test$("opt-ref-rebinding") {
    int foo = 1;
    Opt<int&> foor{Some(foo)};
    int bar = 2;
    foor = Some(bar);

    assertEq$(foo, 1);
    assertEq$(bar, 2);
    assertEq$(foor.expect(), 2);

    return Ok();
}

test$("opt-ref-copy") {
    int value = 42;
    Opt<int&> opt{Some(value)};

    Opt<int&> optCopy = opt;

    assert$(optCopy.has());
    assertEq$(optCopy.expect(), 42);
    assertEq$(&optCopy.expect(), &value);

    optCopy.expect() = 100;
    assertEq$(value, 100);

    return Ok();
}

test$("opt-ref-copy-const") {
    int value = 42;
    Opt<int&> opt{Some(value)};
    Opt<int&> const optCopy = opt;

    assert$(optCopy.has());
    assertEq$(optCopy.expect(), 42);
    assertEq$(&optCopy.expect(), &value);

    optCopy.expect() = 100;
    assertEq$(value, 100);

    return Ok();
}

test$("opt-ref-move") {
    int value = 42;
    Opt<int&> opt{Some(value)};

    Opt<int&> optMoved = std::move(opt);

    assert$(optMoved.has());
    assert$(not opt.has());
    assertEq$(optMoved.expect(), 42);
    assertEq$(&optMoved.expect(), &value);

    optMoved.expect() = 100;
    assertEq$(value, 100);

    return Ok();
}

test$("opt-ref-move-const") {
    int value = 42;
    Opt<int&> opt{Some(value)};
    Opt<int const&> optConst = opt;
    return Ok();
}

} // namespace Karm::Base::Tests
