import Karm.Core;

#include <karm-test/macros.h>

namespace Karm::Async::Tests {

test$("test-cancellation"s) {
    Cancellation cancellation;
    auto ct = cancellation.token();

    Cancellation childCancellation;
    try$(childCancellation.attach(cancellation));

    auto childCt = childCancellation.token();

    expect$(not ct.cancelled());
    expect$(not childCt.cancelled());

    childCancellation.cancel();
    expect$(not ct.cancelled());
    expect$(childCt.cancelled());

    childCancellation.reset();
    expect$(not ct.cancelled());
    expect$(not childCt.cancelled());

    cancellation.cancel();
    expect$(ct.cancelled());
    expect$(childCt.cancelled());

    return Ok();
}

} // namespace Karm::Async::Tests
