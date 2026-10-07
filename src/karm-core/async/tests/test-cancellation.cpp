import Karm.Core;

#include <karm/test>

using namespace Karm::Literals;

namespace Karm::Async::Tests {

test$("cancellation-attach-and-reset"s) {
    Cancellation cancellation;
    auto ct = cancellation.token();

    Cancellation childCancellation;
    try$(childCancellation.attach(cancellation));

    auto childCt = childCancellation.token();

    assert$(not ct.cancelled());
    assert$(not childCt.cancelled());

    childCancellation.cancel();
    assert$(not ct.cancelled());
    assert$(childCt.cancelled());

    childCancellation.reset();
    assert$(not ct.cancelled());
    assert$(not childCt.cancelled());

    cancellation.cancel();
    assert$(ct.cancelled());
    assert$(childCt.cancelled());

    return Ok();
}

} // namespace Karm::Async::Tests
