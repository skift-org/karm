import Karm.Core;
import Karm.Signals;

#include <karm/test>

namespace Karm::Signals::Tests {

// MARK: Signal ----------------------------------------------------------------

test$("signal-get-and-set") {
    Signal n{40};

    expectEq$(n.value(), 40);
    expectEq$(*n, 40);

    n.update(42);

    expectEq$(n.value(), 42);
    expectEq$(n.peek(), 42);

    return Ok();
}

test$("signal-handles-share-one-cell") {
    Signal a{1};
    Signal b = a;

    b.update(2);

    expectEq$(a.value(), 2);

    return Ok();
}

test$("signal-mutate") {
    Signal n{20};

    n.mutate([](int v) {
        return v + 22;
    });

    expectEq$(n.value(), 42);

    return Ok();
}

// MARK: Effect ----------------------------------------------------------------

test$("effect-runs-once-on-creation") {
    Signal n{1};
    usize runs = 0;

    Effect e = [&] {
        runs++;
        (void)n.value();
    };

    expectEq$(runs, 1uz);

    return Ok();
}

test$("effect-reruns-on-write") {
    Signal n{1};
    usize runs = 0;

    Effect e = [&] {
        runs++;
        (void)n.value();
    };

    n.update(2);
    expectEq$(runs, 2uz);

    n.update(3);
    expectEq$(runs, 3uz);

    return Ok();
}

test$("effect-ignores-a-write-of-an-equal-value") {
    Signal n{1};
    usize runs = 0;

    Effect e = [&] {
        runs++;
        (void)n.value();
    };

    n.update(1);
    expectEq$(runs, 1uz);

    return Ok();
}

test$("effect-stops") {
    Signal n{1};
    usize runs = 0;

    Effect e = [&] {
        runs++;
        (void)n.value();
    };

    e.stop();
    n.update(2);

    expectEq$(runs, 1uz);

    return Ok();
}

test$("effect-may-write-signals") {
    Signal src{0};
    Signal mirror{0};
    Vec<int> seen;

    Effect cascade = [&] {
        mirror.update(src.value() * 2);
    };

    Effect watch = [&] {
        seen.pushBack(mirror.value());
    };

    src.update(5);

    expectEq$(mirror.value(), 10);
    expectEq$(seen.len(), 2uz);
    expectEq$(seen[0], 0);
    expectEq$(seen[1], 10);

    return Ok();
}

test$("effect-captured-by-value-stays-callable") {
    Signal n{1};
    usize runs = 0;

    Effect e = [=, &runs] {
        runs++;
        (void)n.value();
    };

    n.update(2);

    expectEq$(runs, 2uz);

    return Ok();
}

test$("effect-destroyed-while-queued-does-not-run") {
    Signal n{1};
    usize runs = 0;

    Opt<Effect> e = [&] {
        runs++;
        (void)n.value();
    };

    batch([&] {
        n.update(2);
        e.take();
    });

    expectEq$(runs, 1uz);

    return Ok();
}

// MARK: Computed --------------------------------------------------------------

test$("computed-is-lazy") {
    Signal n{1};
    usize evals = 0;

    Computed c{[&] {
        evals++;
        return n.value() * 2;
    }};

    expectEq$(evals, 0uz);

    expectEq$(c.value(), 2);
    expectEq$(evals, 1uz);

    n.update(5);
    expectEq$(evals, 1uz);

    expectEq$(c.value(), 10);
    expectEq$(evals, 2uz);

    return Ok();
}

test$("computed-memoizes") {
    Signal n{1};
    usize evals = 0;

    Computed c{[&] {
        evals++;
        return n.value() * 2;
    }};

    expectEq$(c.value(), 2);
    expectEq$(c.value(), 2);
    expectEq$(c.peek(), 2);

    expectEq$(evals, 1uz);

    return Ok();
}

test$("computed-that-yields-an-equal-value-stops-propagation") {
    Signal m{4};
    usize runs = 0;

    Computed isEven{[&] {
        return m.value() % 2 == 0;
    }};

    Effect e = [&] {
        runs++;
        (void)isEven.value();
    };

    m.update(6);
    expectEq$(runs, 1uz);

    m.update(7);
    expectEq$(runs, 2uz);

    return Ok();
}

test$("computed-diamond-is-glitch-free") {
    Signal n{1};
    Vec<int> seen;

    Computed a{[&] {
        return n.value() + 1;
    }};

    Computed b{[&] {
        return n.value() * 10;
    }};

    Computed c{[&] {
        return a.value() + b.value();
    }};

    Effect e = [&] {
        seen.pushBack(c.value());
    };

    n.update(2);

    expectEq$(seen.len(), 2uz);
    expectEq$(seen[0], 12);
    expectEq$(seen[1], 23);

    return Ok();
}

test$("computed-deep-chain") {
    Signal root{1};
    Vec<Computed<int>> chain;

    chain.pushBack(Computed{[&] {
        return root.value() + 1;
    }});

    for (usize i = 1; i < 1000; i++) {
        auto prev = chain[chain.len() - 1];
        chain.pushBack(Computed{[prev] {
            return prev.value() + 1;
        }});
    }

    expectEq$(chain[chain.len() - 1].peek(), 1001);

    root.update(100);

    expectEq$(chain[chain.len() - 1].peek(), 1100);

    return Ok();
}

// MARK: Tracking --------------------------------------------------------------

test$("peek-does-not-subscribe") {
    Signal n{1};
    usize runs = 0;

    Effect e = [&] {
        runs++;
        (void)n.peek();
    };

    n.update(2);

    expectEq$(runs, 1uz);

    return Ok();
}

test$("untrack-does-not-subscribe") {
    Signal hidden{1};
    Signal shown{1};
    usize runs = 0;

    Effect e = [&] {
        runs++;
        untracked([&] {
            return hidden.value();
        });
        (void)shown.value();
    };

    hidden.update(2);
    expectEq$(runs, 1uz);

    shown.update(2);
    expectEq$(runs, 2uz);

    return Ok();
}

test$("dependencies-are-rediscovered-on-every-run") {
    Signal useLeft{true};
    Signal left{1};
    Signal right{100};
    Vec<int> seen;

    Effect e = [&] {
        seen.pushBack(useLeft.value() ? left.value() : right.value());
    };

    right.update(200);
    left.update(2);
    useLeft.update(false);
    left.update(3);
    right.update(300);

    expectEq$(seen.len(), 4uz);
    expectEq$(seen[0], 1);
    expectEq$(seen[1], 2);
    expectEq$(seen[2], 200);
    expectEq$(seen[3], 300);

    return Ok();
}

// MARK: Batch -----------------------------------------------------------------

test$("batch-coalesces-writes") {
    Signal a{1};
    Signal b{2};
    usize runs = 0;

    Effect e = [&] {
        runs++;
        (void)a.value();
        (void)b.value();
    };

    batch([&] {
        a.update(10);
        b.update(20);
    });

    expectEq$(runs, 2uz);

    return Ok();
}

test$("batch-defers-effects-but-not-reads") {
    Signal n{1};
    Vec<int> seen;
    int readInside = 0;
    usize seenInside = 0;

    Effect e = [&] {
        seen.pushBack(n.value());
    };

    batch([&] {
        n.update(2);
        readInside = n.value();
        seenInside = seen.len();
        n.update(3);
    });

    expectEq$(readInside, 2);
    expectEq$(seenInside, 1uz);
    expectEq$(seen.len(), 2uz);
    expectEq$(seen[1], 3);

    return Ok();
}

} // namespace Karm::Signals::Tests
