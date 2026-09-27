import Karm.Core;

#include <karm/test>

namespace Karm::Async::Tests {

test$("karm-semaphore-acquire-release") {
    Semaphore sem{2, 2};

    auto res1 = Async::run(sem.acquireAsync(CancellationToken::uninterruptible()));
    auto res2 = Async::run(sem.acquireAsync(CancellationToken::uninterruptible()));

    assert$(res1);
    assert$(res2);
    assertEq$(sem._currentCount, 0uz);

    sem.release();
    assertEq$(sem._currentCount, 1uz);

    sem.release();
    assertEq$(sem._currentCount, 2uz);

    return Ok();
}

test$("karm-semaphore-release-no-waiters") {
    Semaphore sem{1, 5};

    assertEq$(sem._currentCount, 1uz);

    sem.release(3);

    assertEq$(sem._currentCount, 4uz);

    return Ok();
}

test$("karm-semaphore-release-up-to-max") {
    Semaphore sem{0, 3};

    sem.release(3);

    assertEq$(sem._currentCount, 3uz);

    return Ok();
}

test$("karm-semaphore-wait-then-release") {
    Semaphore sem{0, 2};

    bool res1Ok = false;
    bool res2Ok = false;
    bool res1Done = false;
    bool res2Done = false;
    bool orderOk = false;

    Async::detach(sem.acquireAsync(CancellationToken::uninterruptible()), [&](Res<> res) {
        res1Ok = bool(res);
        res1Done = true;
    });

    Async::detach(sem.acquireAsync(CancellationToken::uninterruptible()), [&](Res<> res) {
        res2Ok = bool(res);
        if (res1Done)
            orderOk = true;
        res2Done = true;
    });

    assert$(not res1Done);
    assert$(not res2Done);

    sem.release(2);

    assert$(orderOk);
    assert$(res1Done);
    assert$(res2Done);
    assert$(res1Ok);
    assert$(res2Ok);
    assertEq$(sem._currentCount, 0uz);

    return Ok();
}

test$("karm-semaphore-partial-release") {
    Semaphore sem{0, 3};

    int completedCount = 0;
    int firstOrder = -1, secondOrder = -1, thirdOrder = -1;

    Async::detach(sem.acquireAsync(CancellationToken::uninterruptible()), [&](Res<>) {
        firstOrder = completedCount++;
    });
    Async::detach(sem.acquireAsync(CancellationToken::uninterruptible()), [&](Res<>) {
        secondOrder = completedCount++;
    });
    Async::detach(sem.acquireAsync(CancellationToken::uninterruptible()), [&](Res<>) {
        thirdOrder = completedCount++;
    });

    sem.release(2);

    assertEq$(completedCount, 2);
    assertEq$(firstOrder, 0);
    assertEq$(secondOrder, 1);
    assertEq$(thirdOrder, -1); // third waiter still queued
    assertEq$(sem._currentCount, 0uz);

    // Releasing the remainder should now unblock the third waiter.
    sem.release(1);

    assertEq$(completedCount, 3);
    assertEq$(thirdOrder, 2);
    assertEq$(sem._currentCount, 0uz);

    return Ok();
}

test$("karm-semaphore-cancel-while-waiting") {
    Semaphore sem{0, 1};

    Cancellation cts;
    bool completed = false;
    bool wasError = false;

    Async::detach(sem.acquireAsync(cts.token()), [&](Res<> res) {
        completed = true;
        wasError = not res;
    });

    assert$(not completed);

    cts.cancel();

    assert$(completed);
    assert$(wasError);

    sem.release();
    assertEq$(sem._currentCount, 1uz);

    return Ok();
}

test$("karm-semaphore-already-cancelled-token") {
    Semaphore sem{0, 1};

    Cancellation cts;
    cts.cancel();

    auto res = Async::run(sem.acquireAsync(cts.token()));

    assert$(not res);
    assertEq$(sem._currentCount, 0uz);

    return Ok();
}

test$("karm-semaphore-destructor-cancels-pending-waiters") {
    bool res1Done = false, res1Err = false;
    bool res2Done = false, res2Err = false;

    {
        Semaphore sem{0, 2};

        Async::detach(sem.acquireAsync(CancellationToken::uninterruptible()), [&](Res<> res) {
            res1Done = true;
            res1Err = not res;
        });
        Async::detach(sem.acquireAsync(CancellationToken::uninterruptible()), [&](Res<> res) {
            res2Done = true;
            res2Err = not res;
        });

        assert$(not res1Done);
        assert$(not res2Done);
    }

    assert$(res1Done);
    assert$(res2Done);
    assert$(res1Err);
    assert$(res2Err);

    return Ok();
}

test$("karm-semaphore-try-lock-scope") {
    Semaphore sem{1, 1};

    {
        auto scope = sem.tryLockScope();
        assert$(scope.has());
        assertEq$(sem._currentCount, 0uz);

        assert$(not sem.tryAcquire());
    }

    assertEq$(sem._currentCount, 1uz);

    return Ok();
}

test$("karm-semaphore-lock-scope-async") {
    Semaphore sem{1, 1};

    {
        auto scope = Async::run(sem.lockScopeAsync(CancellationToken::uninterruptible()));
        assertEq$(sem._currentCount, 0uz);
    }

    assertEq$(sem._currentCount, 1uz);

    return Ok();
}

test$("karm-semaphore-lock-scope-async-cancelled") {
    Semaphore sem{1, 1};

    Cancellation cts;
    cts.cancel();

    auto res = Async::run(sem.lockScopeAsync(cts.token()));

    assert$(not res);
    assertEq$(sem._currentCount, 1uz);

    return Ok();
}

test$("karm-semaphore-lock-scope-move-disarms-source") {
    Semaphore sem{1, 1};

    {
        auto maybeScope1 = sem.tryLockScope();
        assert$(maybeScope1.has());
        assertEq$(sem._currentCount, 0uz);

        auto scope2 = maybeScope1.take();
        assertEq$(sem._currentCount, 0uz);
    }
    assertEq$(sem._currentCount, 1uz);

    return Ok();
}

test$("karm-semaphore-acquire-multiple-sync") {
    Semaphore sem{5, 5};

    auto res1 = Async::run(sem.acquireAsync(3, CancellationToken::uninterruptible()));
    assert$(res1);
    assertEq$(sem._currentCount, 2uz);

    auto res2 = Async::run(sem.acquireAsync(2, CancellationToken::uninterruptible()));
    assert$(res2);
    assertEq$(sem._currentCount, 0uz);

    return Ok();
}

test$("karm-semaphore-try-acquire-multiple") {
    Semaphore sem{5, 5};

    assert$(sem.tryAcquire(3));
    assertEq$(sem._currentCount, 2uz);

    assert$(not sem.tryAcquire(3));
    assertEq$(sem._currentCount, 2uz);

    assert$(sem.tryAcquire(2));
    assertEq$(sem._currentCount, 0uz);

    return Ok();
}

test$("karm-semaphore-acquire-multiple-waits-for-enough") {
    Semaphore sem{0, 10};

    bool done = false;
    bool ok = false;

    Async::detach(sem.acquireAsync(5, CancellationToken::uninterruptible()), [&](Res<> res) {
        done = true;
        ok = bool(res);
    });

    assert$(not done);

    sem.release(3);
    assert$(not done);
    assertEq$(sem._currentCount, 3uz);

    sem.release(2);
    assert$(done);
    assert$(ok);
    assertEq$(sem._currentCount, 0uz);

    return Ok();
}

test$("karm-semaphore-try-acquire-blocked-by-queued-listener") {
    Semaphore sem{0, 10};

    bool done = false;

    Async::detach(sem.acquireAsync(5, CancellationToken::uninterruptible()), [&](Res<>) {
        done = true;
    });

    sem.release(3);
    assert$(not done);
    assertEq$(sem._currentCount, 3uz);

    assert$(not sem.tryAcquire(2));
    assertEq$(sem._currentCount, 3uz);

    sem.release(2);
    assert$(done);
    assertEq$(sem._currentCount, 0uz);

    return Ok();
}

test$("karm-semaphore-fifo-blocks-smaller-later-request") {
    Semaphore sem{0, 10};

    bool firstDone = false;
    bool secondDone = false;

    Async::detach(sem.acquireAsync(3, CancellationToken::uninterruptible()), [&](Res<>) {
        firstDone = true;
    });

    Async::detach(sem.acquireAsync(1, CancellationToken::uninterruptible()), [&](Res<>) {
        secondDone = true;
    });

    sem.release(2);
    assert$(not firstDone);
    assert$(not secondDone);
    assertEq$(sem._currentCount, 2uz);

    sem.release(1);
    assert$(firstDone);
    assert$(not secondDone);
    assertEq$(sem._currentCount, 0uz);

    sem.release(1);
    assert$(secondDone);
    assertEq$(sem._currentCount, 0uz);

    return Ok();
}

} // namespace Karm::Async::Tests