#include <karm/test>

import Karm.Core;

namespace Karm::Base::Tests {

// Basic construction ----------------------------------------------------------

test$("res-ok-basic") {
    Res<int> r = Ok(123);

    expect$(r.has());
    expectEq$(r.unwrap(), 123);

    return Ok();
}

test$("res-err-basic") {
    Error e = Error::invalidInput("broken");
    Res<int> r = e;

    expect$(not r.has());
    expectEq$(r.none().msg(), "broken");

    return Ok();
}

// unwrap / take ---------------------------------------------------------------

test$("res-unwrap-ok") {
    Res<int> r = Ok(7);

    expectEq$(r.unwrap(), 7);

    return Ok();
}

test$("res-take-ok") {
    Res<String> r = Ok<String>("hello");

    auto v = r.take();
    expectEq$(v, "hello");
    // NOTE: take() works like move, Res is in the moved-from Ok state
    expect$(r.has());

    return Ok();
}

// ok() / err() ---------------------------------------------------------------

test$("res-ok-err-access") {
    Res<int> r1 = Ok(42);
    Res<int> r2 = Error::other("nope");

    expect$(r1.ok().has());
    expect$(not r1.error().has());

    expect$(not r2.ok().has());
    expect$(r2.error().has());
    expectEq$(r2.error().unwrap().msg(), "nope");

    return Ok();
}

// unwrapOr / unwrapOrElse -----------------------------------------------------

test$("res-unwrap-or") {
    Res<int> r1 = Ok(10);
    Res<int> r2 = Error::other("x");

    expectEq$(r1.unwrapOr(99), 10);
    expectEq$(r2.unwrapOr(99), 99);

    return Ok();
}

test$("res-unwrap-or-else") {
    Res<int> r1 = Ok(5);
    Res<int> r2 = Error::other("dead");

    expectEq$(r1.unwrapOrElse([] {
        return 999;
    }),
              5);
    expectEq$(r2.unwrapOrElse([] {
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

    expect$(r2.has());
    expectEq$(r2.unwrap(), 6);

    return Ok();
}

test$("res-map-err") {
    Res<int> r = Error::other("boom");

    auto r2 = r.map<int>([](int v) {
        return v * 3;
    });

    expect$(not r2.has());
    expectEq$(r2.error().unwrap().msg(), "boom");

    return Ok();
}

// mapErr() --------------------------------------------------------------------

test$("res-map-err-transform") {
    Res<int> r = Error::invalidInput("old");

    auto r2 = r.mapErr<Error>([](auto const&) {
        return Error::other("new:old");
    });

    expect$(not r2.has());
    expectEq$(r2.error().unwrap().msg(), "new:old");

    return Ok();
}

test$("res-map-err-ok") {
    Res<int> r = Ok(12);

    auto r2 = r.mapErr<Error>([](auto const&) {
        panic("should never be called");
        return Error::other("x");
    });

    expect$(r2.has());
    expectEq$(r2.unwrap(), 12);

    return Ok();
}

// Basic Ok<T&> construction ---------------------------------------------------

test$("ok-ref-basic") {
    int value = 123;
    Ok<int&> o{value};

    expect$(bool(o));
    expectEq$(o.unwrap(), 123);

    o.unwrap() = 999;
    expectEq$(value, 999);

    return Ok();
}

test$("ok-ref-take") {
    int value = 10;
    Ok<int&> o{value};

    int& r = o.take();
    expectEq$(r, 10);

    r = 20;
    expectEq$(value, 20);

    return Ok();
}

// Res<T&> holding reference ---------------------------------------------------

test$("res-ref-ok") {
    int v = 7;
    Res<int&> r = Ok<int&>(v);

    expect$(r.has());
    expectEq$(r.unwrap(), 7);

    r.unwrap() = 42;
    expectEq$(v, 42);

    return Ok();
}

test$("res-ref-take") {
    int v = 5;
    Res<int&> r = Ok<int&>(v);

    int& ref = r.take();
    expectEq$(ref, 5);

    ref = 99;
    expectEq$(v, 99);

    return Ok();
}

// unwrapOr / unwrapOrElse should still return a *value*, not a ref -----------

test$("res-ref-unwrapOr") {
    int v = 1;
    Res<int&> r1 = Ok<int&>(v);
    Res<int&> r2 = Error::other("nope");

    expectEq$(r1.unwrapOr(111), 1);
    expectEq$(r2.unwrapOr(111), 111);

    // ensure unwrapOr doesn't modify original because it returns by value
    v = 20;
    expectEq$(r1.unwrapOr(111), 20);

    return Ok();
}

test$("res-ref-unwrapOrElse") {
    int v = 10;
    Res<int&> r1 = Ok<int&>(v);
    Res<int&> r2 = Error::other("err");

    expectEq$(r1.unwrapOrElse([] {
        return 500;
    }),
              10);
    expectEq$(r2.unwrapOrElse([] {
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

    expect$(r2.has());
    expectEq$(r2.unwrap(), 12);

    // check original still modifiable and referenced
    v = 7;
    expectEq$(r.unwrap(), 7);

    return Ok();
}

// mapErr() should pass through Error unchanged -------------------------------

test$("res-ref-mapErr") {
    Res<int&> r = Error::invalidInput("bad");

    auto r2 = r.mapErr<Error>([](auto const&) {
        return Error::other("new:bad");
    });

    expect$(not r2.has());
    expectEq$(r2.error().unwrap().msg(), "new:bad");

    return Ok();
}

// Converting Res<U&,E> -> Res<V,E> constructor path ---------------------------

test$("res-ref-conversion") {
    int v = 44;
    Res<int&> r1 = Ok<int&>(v);

    Res<int> r2 = r1; // should copy value

    expect$(r2.has());
    expectEq$(r2.unwrap(), 44);

    v = 200;
    expectEq$(r2.unwrap(), 44); // ensure decoupling

    return Ok();
}

} // namespace Karm::Base::Tests
