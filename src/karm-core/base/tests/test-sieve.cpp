import Karm.Core;

#include <karm/test>

namespace Karm::Base::Tests {

test$("sieve-access") {
    Sieve<int, int> cache{10};

    for (int i = 0; i < 10; i++) {
        (void)cache.access(i, [&] {
            return i * 10;
        });
    }

    for (int i = 0; i < 10; i++) {
        auto val = cache.get(i);
        assertEq$(val, i * 10);
    }

    return Ok();
}

test$("sieve-contains") {
    Sieve<int, int> cache{10};
    assert$(not cache.has(0));
    (void)cache.access(0, [&] {
        return 0;
    });
    assert$(cache.has(0));
    return Ok();
}

test$("sieve-len") {
    Sieve<int, int> cache{10};
    assertEq$(cache.len(), 0uz);
    (void)cache.access(0, [&] {
        return 0;
    });
    assertEq$(cache.len(), 1uz);

    for (int i = 1; i < 10; i++) {
        (void)cache.access(i, [&] {
            return i * 10;
        });
    }

    assertEq$(cache.len(), 10uz);
    return Ok();
}

test$("sieve-evict") {
    Sieve<int, int> cache{10};

    for (int i = 0; i < 10; i++) {
        (void)cache.access(i, [&] {
            return i * 10;
        });
    }

    for (int i = 0; i < 10; i++) {
        auto val = cache.get(i);
        assertEq$(val, i * 10);
    }

    (void)cache.access(10, [&] {
        return 10 * 10;
    });

    assert$(not cache.has(0));
    assertEq$(cache.len(), 10uz);

    return Ok();
}

} // namespace Karm::Base::Tests
