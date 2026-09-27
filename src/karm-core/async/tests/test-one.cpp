import Karm.Core;

#include <karm/test>

namespace Karm::Async::Tests {

test$("sender-one") {
    auto sender = Async::One<int>{10};
    auto res = Async::run(sender);
    assertEq$(res, 10);
    return Ok();
}

} // namespace Karm::Async::Tests
