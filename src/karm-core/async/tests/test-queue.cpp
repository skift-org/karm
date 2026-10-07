import Karm.Core;

#include <karm/test>

namespace Karm::Async::Tests {

test$("queue-enqueue-dequeue") {
    Queue<isize> q;
    q.enqueue(42);
    q.enqueue(69);

    auto res1 = Async::run(q.dequeueAsync(CancellationToken::uninterruptible()));
    auto res2 = Async::run(q.dequeueAsync(CancellationToken::uninterruptible()));

    assertEq$(res1, 42);
    assertEq$(res2, 69);

    return Ok();
}

test$("queue-dequeue-enqueue") {
    Queue<isize> q;

    isize res1 = 0;
    isize res2 = 0;
    bool orderOk = false;

    Async::detach(q.dequeueAsync(CancellationToken::uninterruptible()), [&](Res<isize> res) {
        res1 = res.expect();
    });

    Async::detach(q.dequeueAsync(CancellationToken::uninterruptible()), [&](Res<isize> res) {
        if (res1 == 42)
            orderOk = true;
        res2 = res.expect();
    });

    q.enqueue(42);
    q.enqueue(69);
    q.enqueue(96);

    assert$(orderOk);
    assertEq$(res1, 42);
    assertEq$(res2, 69);

    assert$(not q.empty());
    assertEq$(q.tryDequeue(), 96);

    return Ok();
}

} // namespace Karm::Async::Tests