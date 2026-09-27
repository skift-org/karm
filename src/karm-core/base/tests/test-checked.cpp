import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("will-add-overflow") {
    assert$(willAddOverflow<u32>(0xFFFFFFFF, 1));
    assert$(willAddOverflow<i32>(Limits<i32>::MAX, 1));

    return Ok();
}

test$("will-add-underflow") {
    assert$(willAddUnderflow<i32>(Limits<i32>::MIN, -1));

    return Ok();
}

test$("will-sub-overflow") {
    assert$(willSubOverflow<u32>(0, -1));
    assert$(willSubOverflow<i32>(Limits<i32>::MAX, -1));

    return Ok();
}

test$("will-sub-underflow") {
    assert$(willSubUnderflow<u32>(0, 1));
    assert$(willSubUnderflow<i32>(Limits<i32>::MIN, 1));

    return Ok();
}

} // namespace Karm::Base::Tests
