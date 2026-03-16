import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("strong-rc") {
    struct S {
        int x = 0;
    };

    auto s = makeRc<S>();

    return Ok();
}

test$("weak-self") {
    struct Foo {
        Opt<Karm::Weak<Foo>> _self;

        Rc<Foo> self() {
            return _self
                .unwrap("self reference not binded")
                .upgrade()
                .unwrap();
        }
    };

    auto foo = makeRc<Foo>();
    foo->_self = foo;
    auto foo2 = foo->self();

    expectEq$(foo.strong(), 2uz);
    expectEq$(foo2.strong(), 2uz);
    expectEq$(foo2.weak(), 1uz);

    return Ok();
}

test$("rc-niche") {
    Opt<Rc<int>> test;

    expectEq$(sizeof(test), sizeof(Rc<int>));
    expectEq$(test.has(), false);
    expectEq$(test, NONE);
    test = makeRc<int>(5);
    expectEq$(test.unwrap(), 5);
    expectEq$(test.take(), 5);
    expectEq$(test, NONE);
    test = makeRc<int>();
    expectEq$(test.has(), true);

    return Ok();
}

test$("rc-same-instance") {
    auto a = makeRc(1);
    auto b = makeRc(1);
    auto c = a;

    expect$(a.sameInstance(c));
    expect$(not a.sameInstance(b));
    expect$(not c.sameInstance(b));

    return Ok();
}

} // namespace Karm::Base::Tests
