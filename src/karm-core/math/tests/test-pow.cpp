#include <karm/test>

import Karm.Core;

namespace Karm::Math::Tests {

template <Meta::Float T>
static Res<> _testPow(auto& _driver, T epsilon) {
    assert$(epsilonEq<T>(pow<T>(2, 10), 1024, epsilon));
    assert$(epsilonEq<T>(pow<T>(2, -2), 0.25, epsilon));
    assert$(epsilonEq<T>(pow<T>(-2, 3), -8, epsilon));
    assert$(epsilonEq<T>(pow<T>(-2, 2), 4, epsilon));
    assert$(epsilonEq<T>(pow<T>(9, 0.5), 3, epsilon));
    assert$(epsilonEq<T>(pow<T>(2, 0.5), SQRT2, epsilon));
    assert$(epsilonEq<T>(pow<T>(10, 2.5), 316.22776601683796, epsilon));
    assert$(epsilonEq<T>(pow<T>(0.5, 3.25), 0.10511205190671431, epsilon));

    assertEq$(pow<T>(1, NAN), T{1});
    assertEq$(pow<T>(NAN, 0), T{1});
    assertEq$(pow<T>(0, 2), T{0});
    assert$(isNan(pow<T>(NAN, 2)));
    assert$(isNan(pow<T>(-2, 0.5)));
    assert$(isInf(pow<T>(INF, 2)));
    assertEq$(pow<T>(INF, -2), T{0});

    return Ok();
}

test$("pow-f32") {
    return _testPow<f32>(_driver, 1e-4);
}

test$("pow-f64") {
    return _testPow<f64>(_driver, 1e-12);
}

} // namespace Karm::Math::Tests
