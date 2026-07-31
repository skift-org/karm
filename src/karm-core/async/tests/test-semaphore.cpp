import Karm.Core;

#include <karm/test>

namespace Karm::Async::Tests {

test$("karm-semaphore-acquire-release") {
    Semaphore sem{2, 2};

    auto res1 = Async::run(sem.acquireAsync(CancellationToken::uninterruptible()));
    auto res2 = Async::run(sem.acquireAsync(CancellationToken::uninterruptible()));

    expect$(res1);
    expect$(res2);
    expectEq$(sem._currentCount, 0uz);

    sem.release();
    expectEq$(sem._currentCount, 1uz);

    sem.release();
    expectEq$(sem._currentCount, 2uz);

    return Ok();
}

test$("karm-semaphore-release-no-waiters") {
    Semaphore sem{1, 5};

    expectEq$(sem._currentCount, 1uz);

    sem.release(3);

    expectEq$(sem._currentCount, 4uz);

    return Ok();
}

test$("karm-semaphore-release-up-to-max") {
    Semaphore sem{0, 3};

    sem.release(3);

    expectEq$(sem._currentCount, 3uz);

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

    expect$(not res1Done);
    expect$(not res2Done);

    sem.release(2);

    expect$(orderOk);
    expect$(res1Done);
    expect$(res2Done);
    expect$(res1Ok);
    expect$(res2Ok);
    expectEq$(sem._currentCount, 0uz);

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

    expectEq$(completedCount, 2);
    expectEq$(firstOrder, 0);
    expectEq$(secondOrder, 1);
    expectEq$(thirdOrder, -1); // third waiter still queued
    expectEq$(sem._currentCount, 0uz);

    // Releasing the remainder should now unblock the third waiter.
    sem.release(1);

    expectEq$(completedCount, 3);
    expectEq$(thirdOrder, 2);
    expectEq$(sem._currentCount, 0uz);

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

    expect$(not completed);

    cts.cancel();

    expect$(completed);
    expect$(wasError);

    sem.release();
    expectEq$(sem._currentCount, 1uz);

    return Ok();
}

test$("karm-semaphore-already-cancelled-token") {
    Semaphore sem{0, 1};

    Cancellation cts;
    cts.cancel();

    auto res = Async::run(sem.acquireAsync(cts.token()));

    expect$(not res);
    expectEq$(sem._currentCount, 0uz);

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

        expect$(not res1Done);
        expect$(not res2Done);
    }

    expect$(res1Done);
    expect$(res2Done);
    expect$(res1Err);
    expect$(res2Err);

    return Ok();
}

test$("karm-semaphore-try-lock-scope") {
    Semaphore sem{1, 1};

    {
        auto scope = sem.tryLockScope();
        expect$(scope.has());
        expectEq$(sem._currentCount, 0uz);

        expect$(not sem.tryAcquire());
    }

    expectEq$(sem._currentCount, 1uz);

    return Ok();
}

test$("karm-semaphore-lock-scope-async") {
    Semaphore sem{1, 1};

    {
        auto scope = Async::run(sem.lockScopeAsync(CancellationToken::uninterruptible()));
        expectEq$(sem._currentCount, 0uz);
    }

    expectEq$(sem._currentCount, 1uz);

    return Ok();
}

test$("karm-semaphore-lock-scope-async-cancelled") {
    Semaphore sem{1, 1};

    Cancellation cts;
    cts.cancel();

    auto res = Async::run(sem.lockScopeAsync(cts.token()));

    expect$(not res);
    expectEq$(sem._currentCount, 1uz);

    return Ok();
}

test$("karm-semaphore-lock-scope-move-disarms-source") {
    Semaphore sem{1, 1};

    {
        auto maybeScope1 = sem.tryLockScope();
        expect$(maybeScope1.has());
        expectEq$(sem._currentCount, 0uz);

        auto scope2 = maybeScope1.take();
        expectEq$(sem._currentCount, 0uz);

    }
    expectEq$(sem._currentCount, 1uz);

    return Ok();
}

} // namespace Karm::Async::Tests