#include <karm/test>

import Karm.Core;

using namespace Karm::Literals;

namespace Karm::Base::Tests {

// Basic construction ----------------------------------------------------------

test$("res-ok-basic") {
    Res<int> r = Ok(123);

    assert$(r.has());
    assertEq$(r.expect(), 123);

    return Ok();
}

test$("res-err-basic") {
    Error e = Error::invalidInput("broken");
    Res<int> r = e;

    assert$(not r.has());
    assertEq$(r.none().msg(), "broken"s);

    return Ok();
}

// unwrap / take ---------------------------------------------------------------

test$("res-unwrap-ok") {
    Res<int> r = Ok(7);

    assertEq$(r.expect(), 7);

    return Ok();
}

test$("res-take-ok") {
    Res<String> r = Ok<String>("hello");

    auto v = r.take();
    assertEq$(v, "hello"s);
    // NOTE: take() works like move, Res is in the moved-from Ok state
    assert$(r.has());

    return Ok();
}

// ok() / err() ---------------------------------------------------------------

test$("res-ok-err-access") {
    Res<int> r1 = Ok(42);
    Res<int> r2 = Error::other("nope");

    assert$(r1.ok().has());
    assert$(not r1.error().has());

    assert$(not r2.ok().has());
    assert$(r2.error().has());
    assertEq$(r2.error().expect().msg(), "nope"s);

    return Ok();
}

// unwrapOr / unwrapOrElse -----------------------------------------------------

test$("res-unwrap-or") {
    Res<int> r1 = Ok(10);
    Res<int> r2 = Error::other("x");

    assertEq$(r1.unwrapOr(99), 10);
    assertEq$(r2.unwrapOr(99), 99);

    return Ok();
}

test$("res-unwrap-or-else") {
    Res<int> r1 = Ok(5);
    Res<int> r2 = Error::other("dead");

    assertEq$(r1.unwrapOrElse([] {
        return 999;
    }),
              5);
    assertEq$(r2.unwrapOrElse([] {
        return 999;
    }),
              999);

    return Ok();
}

// map() -----------------------------------------------------------------------

test$("res-map-ok") {
    Res<int> r = Ok(2);

    auto r2 = r.map<int>([](int v) {
        return v * 3;
    });

    assert$(r2.has());
    assertEq$(r2.expect(), 6);

    return Ok();
}

test$("res-map-err") {
    Res<int> r = Error::other("boom");

    auto r2 = r.map<int>([](int v) {
        return v * 3;
    });

    assert$(not r2.has());
    assertEq$(r2.error().expect().msg(), "boom"s);

    return Ok();
}

// mapErr() --------------------------------------------------------------------

test$("res-map-err-transform") {
    Res<int> r = Error::invalidInput("old");

    auto r2 = r.mapErr<Error>([](auto const&) {
        return Error::other("new:old");
    });

    assert$(not r2.has());
    assertEq$(r2.error().expect().msg(), "new:old"s);

    return Ok();
}

test$("res-map-err-ok") {
    Res<int> r = Ok(12);

    auto r2 = r.mapErr<Error>([](auto const&) {
        panic("should never be called");
        return Error::other("x");
    });

    assert$(r2.has());
    assertEq$(r2.expect(), 12);

    return Ok();
}

// Basic Ok<T&> construction ---------------------------------------------------

test$("ok-ref-basic") {
    int value = 123;
    Ok<int&> o{value};

    assert$(bool(o));
    assertEq$(o.unwrap(), 123);

    o.unwrap() = 999;
    assertEq$(value, 999);

    return Ok();
}

test$("ok-ref-take") {
    int value = 10;
    Ok<int&> o{value};

    int& r = o.take();
    assertEq$(r, 10);

    r = 20;
    assertEq$(value, 20);

    return Ok();
}

// Res<T&> holding reference ---------------------------------------------------

test$("res-ref-ok") {
    int v = 7;
    Res<int&> r = Ok<int&>(v);

    assert$(r.has());
    assertEq$(r.expect(), 7);

    r.expect() = 42;
    assertEq$(v, 42);

    return Ok();
}

test$("res-ref-take") {
    int v = 5;
    Res<int&> r = Ok<int&>(v);

    int& ref = r.take();
    assertEq$(ref, 5);

    ref = 99;
    assertEq$(v, 99);

    return Ok();
}

// unwrapOr / unwrapOrElse should still return a *value*, not a ref -----------

test$("res-ref-unwrap-or") {
    int v = 1;
    Res<int&> r1 = Ok<int&>(v);
    Res<int&> r2 = Error::other("nope");

    assertEq$(r1.unwrapOr(111), 1);
    assertEq$(r2.unwrapOr(111), 111);

    // ensure unwrapOr doesn't modify original because it returns by value
    v = 20;
    assertEq$(r1.unwrapOr(111), 20);

    return Ok();
}

test$("res-ref-unwrap-or-else") {
    int v = 10;
    Res<int&> r1 = Ok<int&>(v);
    Res<int&> r2 = Error::other("err");

    assertEq$(r1.unwrapOrElse([] {
        return 500;
    }),
              10);
    assertEq$(r2.unwrapOrElse([] {
        return 500;
    }),
              500);

    return Ok();
}

// map() should not break the reference behavior -------------------------------

test$("res-ref-map") {
    int v = 3;
    Res<int&> r = Ok<int&>(v);

    auto r2 = r.map<int>([](int& x) {
        return x * 4;
    });

    assert$(r2.has());
    assertEq$(r2.expect(), 12);

    // check original still modifiable and referenced
    v = 7;
    assertEq$(r.expect(), 7);

    return Ok();
}

// mapErr() should pass through Error unchanged -------------------------------

test$("res-ref-map-err") {
    Res<int&> r = Error::invalidInput("bad");

    auto r2 = r.mapErr<Error>([](auto const&) {
        return Error::other("new:bad");
    });

    assert$(not r2.has());
    assertEq$(r2.error().expect().msg(), "new:bad"s);

    return Ok();
}

// Converting Res<U&,E> -> Res<V,E> constructor path ---------------------------

test$("res-ref-conversion") {
    int v = 44;
    Res<int&> r1 = Ok<int&>(v);

    Res<int> r2 = r1; // should copy value

    assert$(r2.has());
    assertEq$(r2.expect(), 44);

    v = 200;
    assertEq$(r2.expect(), 44); // ensure decoupling

    return Ok();
}

} // namespace Karm::Base::Tests
