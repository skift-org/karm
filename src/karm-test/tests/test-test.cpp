#include <karm/test>

namespace Karm::Test::Tests {

test$("test-ok") {
    return Ok();
}

test$("test-xfail", XFAIL) {
    assertEq$(1, 2);

    return Ok();
}

} // namespace Karm::Test::Tests
