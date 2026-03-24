#include <karm/test>

import Karm.Core;

namespace Karm::Base::Tests {

test$("bloom") {
    Bloom<int> bloom;

    bloom.add(42);

    expect$(bloom.maybeContains(42));
    expect$(not bloom.maybeContains(43));

    return Ok();
}

} // namespace Karm::Base::Tests
