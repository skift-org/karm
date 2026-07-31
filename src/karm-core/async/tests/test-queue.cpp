import Karm.Core;

#include <karm/test>

namespace Karm::Async::Tests {

test$("karm-queue-enqueue-dequeue") {
    Queue<isize> q;
    q.enqueue(42);
    q.enqueue(69);

    auto res1 = Async::run(q.dequeueAsync(CancellationToken::uninterruptible()));
    auto res2 = Async::run(q.dequeueAsync(CancellationToken::uninterruptible()));

    expectEq$(res1, 42);
    expectEq$(res2, 69);

    return Ok();
}

test$("karm-queue-dequeue-enqueue") {
    Queue<isize> q;

    isize res1 = 0;
    isize res2 = 0;
    bool orderOk = false;

    Async::detach(q.dequeueAsync(CancellationToken::uninterruptible()), [&](Res<isize> res) {
        res1 = res.unwrap();
    });

    Async::detach(q.dequeueAsync(CancellationToken::uninterruptible()), [&](Res<isize> res) {
        if (res1 == 42)
            orderOk = true;
        res2 = res.unwrap();
    });

    q.enqueue(42);
    q.enqueue(69);
    q.enqueue(96);

    expect$(orderOk);
    expectEq$(res1, 42);
    expectEq$(res2, 69);

    expect$(not q.empty());
    expectEq$(q.tryDequeue(), 96);

    return Ok();
}

} // namespace Karm::Async::Tests