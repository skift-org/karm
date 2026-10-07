#include <karm/test>

import Karm.Core;

namespace Karm::Base::Tests {

test$("bloom-basic") {
    Bloom<int> bloom;

    bloom.add(42);

    assert$(bloom.maybeContains(42));
    assert$(not bloom.maybeContains(43));

    return Ok();
}

test$("counting-bloom-add-and-remove") {
    CountingBloom<u64> bloom;

    assert$(not bloom.maybeContains(0xabc));

    bloom.add(0xabc);
    assert$(bloom.maybeContains(0xabc));

    bloom.remove(0xabc);
    assert$(not bloom.maybeContains(0xabc));

    return Ok();
}

test$("counting-bloom-nests") {
    CountingBloom<u64> bloom;

    bloom.add(0x1234);
    bloom.add(0x1234);
    bloom.remove(0x1234);
    assert$(bloom.maybeContains(0x1234));

    bloom.remove(0x1234);
    assert$(not bloom.maybeContains(0x1234));

    return Ok();
}

test$("counting-bloom-never-false-negative") {
    CountingBloom<u64> bloom;

    for (u64 i = 0; i < 200; i++)
        bloom.add(i);

    for (u64 i = 0; i < 200; i++)
        assert$(bloom.maybeContains(i));

    return Ok();
}

} // namespace Karm::Base::Tests
