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
                .expect("self reference not binded")
                .upgrade()
                .expect();
        }
    };

    auto foo = makeRc<Foo>();
    foo->_self = Some(foo);
    auto foo2 = foo->self();

    assertEq$(foo.strong(), 2uz);
    assertEq$(foo2.strong(), 2uz);
    assertEq$(foo2.weak(), 2uz);

    return Ok();
}

test$("rc-niche") {
    Opt<Rc<int>> test;

    assertEq$(sizeof(test), sizeof(Rc<int>));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(makeRc<int>(5));
    assertEq$(test.expect(), 5);
    assertEq$(test.take(), 5);
    assertEq$(test, NONE);
    test = Some(makeRc<int>());
    assertEq$(test.has(), true);

    return Ok();
}

test$("rc-same-instance") {
    auto a = makeRc(1);
    auto b = makeRc(1);
    auto c = a;

    assert$(a.sameInstance(c));
    assert$(not a.sameInstance(b));
    assert$(not c.sameInstance(b));

    return Ok();
}

} // namespace Karm::Base::Tests
