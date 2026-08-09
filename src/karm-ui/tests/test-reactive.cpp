import Karm.Core;
import Karm.Ui;
import Karm.Signals;

#include <karm/test>

namespace Karm::Ui::Tests {

test$("reactive-builds-lazily") {
    Signals::Signal n{0};
    usize builds = 0;

    auto node = reactive([&] {
        builds++;
        (void)n.value();
        return empty();
    });

    expectEq$(builds, 0uz);

    node->layout({});
    expectEq$(builds, 1uz);

    node->layout({});
    expectEq$(builds, 1uz);

    return Ok();
}

test$("reactive-rebuilds-when-a-signal-it-read-changes") {
    Signals::Signal n{0};
    usize builds = 0;

    auto node = reactive([&] {
        builds++;
        (void)n.value();
        return empty();
    });

    node->layout({});
    expectEq$(builds, 1uz);

    n.update(1);
    node->layout({});
    expectEq$(builds, 2uz);

    return Ok();
}

test$("reactive-does-not-rebuild-during-the-write") {
    Signals::Signal n{0};
    usize builds = 0;

    auto node = reactive([&] {
        builds++;
        (void)n.value();
        return empty();
    });

    node->layout({});
    expectEq$(builds, 1uz);

    n.update(1);
    expectEq$(builds, 1uz);

    node->layout({});
    expectEq$(builds, 2uz);

    return Ok();
}

test$("reactive-coalesces-several-writes-into-one-rebuild") {
    Signals::Signal a{0};
    Signals::Signal b{0};
    usize builds = 0;

    auto node = reactive([&] {
        builds++;
        (void)a.value();
        (void)b.value();
        return empty();
    });

    node->layout({});

    a.update(1);
    b.update(2);
    a.update(3);

    node->layout({});
    expectEq$(builds, 2uz);

    return Ok();
}

test$("reactive-ignores-signals-it-did-not-read") {
    Signals::Signal read{0};
    Signals::Signal unread{0};
    usize builds = 0;

    auto node = reactive([&] {
        builds++;
        (void)read.value();
        return empty();
    });

    node->layout({});

    unread.update(1);
    node->layout({});
    expectEq$(builds, 1uz);

    read.update(1);
    node->layout({});
    expectEq$(builds, 2uz);

    return Ok();
}

test$("reactive-resubscribes-on-every-build") {
    Signals::Signal useLeft{true};
    Signals::Signal left{1};
    Signals::Signal right{100};
    usize builds = 0;

    auto node = reactive([&] {
        builds++;
        (void)(useLeft.value() ? left.value() : right.value());
        return empty();
    });

    node->layout({});
    expectEq$(builds, 1uz);

    right.update(200);
    node->layout({});
    expectEq$(builds, 1uz);

    left.update(2);
    node->layout({});
    expectEq$(builds, 2uz);

    useLeft.update(false);
    node->layout({});
    expectEq$(builds, 3uz);

    left.update(3);
    node->layout({});
    expectEq$(builds, 3uz);

    right.update(300);
    node->layout({});
    expectEq$(builds, 4uz);

    return Ok();
}

test$("reactive-sees-a-computed-through-the-graph") {
    Signals::Signal n{4};
    usize builds = 0;

    Signals::Computed isEven{[&] {
        return n.value() % 2 == 0;
    }};

    auto node = reactive([&] {
        builds++;
        (void)isEven.value();
        return empty();
    });

    node->layout({});
    expectEq$(builds, 1uz);

    n.update(6);
    node->layout({});
    expectEq$(builds, 1uz);

    n.update(7);
    node->layout({});
    expectEq$(builds, 2uz);

    return Ok();
}

test$("reactive-unsubscribes-when-destroyed") {
    Signals::Signal n{0};
    usize builds = 0;

    {
        auto node = reactive([&] {
            builds++;
            (void)n.value();
            return empty();
        });
        node->layout({});
    }

    n.update(1);

    expectEq$(builds, 1uz);

    return Ok();
}

test$("reactive-keeps-node-at-offset-zero") {
    Signals::Signal n{0};

    auto node = reactive([&] {
        (void)n.value();
        return empty();
    });

    auto casted = node.cast<Reactive>();
    expect$(casted.has());

    auto& view = *casted.unwrap();

    expectEq$((void*)static_cast<Node*>(&view), (void*)&view);
    expectNe$((void*)static_cast<Signals::Node*>(&view), (void*)&view);

    return Ok();
}

// MARK: Nesting ---------------------------------------------------------------

test$("nested-reactive-tracks-each-level-separately") {
    Signals::Signal outerSig{0};
    Signals::Signal innerSig{0};
    usize outerBuilds = 0;
    usize innerBuilds = 0;

    auto node = reactive([&] {
        outerBuilds++;
        (void)outerSig.value();

        return reactive([&] {
            innerBuilds++;
            (void)innerSig.value();
            return empty();
        });
    });

    node->layout({});
    expectEq$(outerBuilds, 1uz);
    expectEq$(innerBuilds, 1uz);

    innerSig.update(1);
    node->layout({});
    expectEq$(outerBuilds, 1uz);
    expectEq$(innerBuilds, 2uz);

    return Ok();
}

test$("outer-rebuild-drags-the-inner-with-it") {
    Signals::Signal outerSig{0};
    Signals::Signal innerSig{0};
    usize outerBuilds = 0;
    usize innerBuilds = 0;

    auto node = reactive([&] {
        outerBuilds++;
        (void)outerSig.value();

        return reactive([&] {
            innerBuilds++;
            (void)innerSig.value();
            return empty();
        });
    });

    node->layout({});

    outerSig.update(1);
    node->layout({});

    expectEq$(outerBuilds, 2uz);
    expectEq$(innerBuilds, 2uz);

    return Ok();
}

test$("nested-reactive-built-during-the-outer-build-still-tracks-separately") {
    Signals::Signal outerSig{0};
    Signals::Signal innerSig{0};
    usize outerBuilds = 0;
    usize innerBuilds = 0;

    auto node = reactive([&] {
        outerBuilds++;
        (void)outerSig.value();

        auto inner = reactive([&] {
            innerBuilds++;
            (void)innerSig.value();
            return empty();
        });

        (void)inner->size({}, Hint::MIN);

        return inner;
    });

    node->layout({});
    expectEq$(outerBuilds, 1uz);
    expectEq$(innerBuilds, 1uz);

    innerSig.update(1);
    node->layout({});
    expectEq$(outerBuilds, 1uz);
    expectEq$(innerBuilds, 2uz);

    return Ok();
}

test$("a-reconciled-away-child-takes-its-subscriptions-with-it") {
    Signals::Signal outerSig{0};
    Signals::Signal innerSig{0};
    usize outerBuilds = 0;
    usize innerBuilds = 0;

    auto node = reactive([&] {
        outerBuilds++;
        (void)outerSig.value();

        auto inner = reactive([&] {
            innerBuilds++;
            (void)innerSig.value();
            return empty();
        });

        (void)inner->size({}, Hint::MIN);

        return inner;
    });

    node->layout({});
    expectEq$(innerBuilds, 1uz);

    outerSig.update(1);
    node->layout({});
    expectEq$(outerBuilds, 2uz);
    expectEq$(innerBuilds, 3uz);

    innerSig.update(1);
    node->layout({});
    expectEq$(outerBuilds, 2uz);
    expectEq$(innerBuilds, 4uz);

    return Ok();
}

} // namespace Karm::Ui::Tests
