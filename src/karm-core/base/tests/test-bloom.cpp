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

test$("counting-bloom-add-and-remove") {
    CountingBloom<u64> bloom;

    expect$(not bloom.maybeContains(0xabc));

    bloom.add(0xabc);
    expect$(bloom.maybeContains(0xabc));

    bloom.remove(0xabc);
    expect$(not bloom.maybeContains(0xabc));

    return Ok();
}

test$("counting-bloom-nests") {
    CountingBloom<u64> bloom;

    bloom.add(0x1234);
    bloom.add(0x1234);
    bloom.remove(0x1234);
    expect$(bloom.maybeContains(0x1234));

    bloom.remove(0x1234);
    expect$(not bloom.maybeContains(0x1234));

    return Ok();
}

test$("counting-bloom-never-false-negative") {
    CountingBloom<u64, 64> bloom;

    for (u64 i = 0; i < 200; i++)
        bloom.add(i);

    for (u64 i = 0; i < 200; i++)
        expect$(bloom.maybeContains(i));

    return Ok();
}

} // namespace Karm::Base::Tests
