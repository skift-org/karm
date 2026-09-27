import Karm.Core;

#include <karm/test>

namespace Karm::Math::Tests {

test$("f64-niche") {
    Opt<f64> test;

    assertEq$(sizeof(test), sizeof(f64));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(5);
    assertEq$(test.expect(), 5);
    assertEq$(test.take(), 5);
    assertEq$(test, NONE);
    test = Some(Math::NAN);
    assertEq$(test.has(), true);
    test = Some(-Math::NAN);
    assertEq$(test.has(), true);

    return Ok();
}

test$("f32-niche") {
    Opt<f32> test;

    assertEq$(sizeof(test), sizeof(f32));
    assertEq$(test.has(), false);
    assertEq$(test, NONE);
    test = Some(5);
    assertEq$(test.expect(), 5);
    assertEq$(test.take(), 5);
    assertEq$(test, NONE);

    f32 const NAN = 0.0f / 0.0f;
    f32 const INF = 1.0f / 0.0f;
    f32 const NEG_INF = -1.0f / 0.0f;
    Array<f32, 10> values = {
        NAN,
        NEG_INF,
        NAN,
        -NAN,
        INF * 0.0f,
        NEG_INF * 0.0f,
        0.0f / 0.0f,
        0.0f / (-0.0f),
        INF / INF,
        INF / NEG_INF,
    };
    for (auto val : values) {
        test = Some(val);
        assertEq$(test.has(), true);
    }

    return Ok();
}

} // namespace Karm::Math::Tests
