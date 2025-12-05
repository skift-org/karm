import Karm.Core;

#include <karm-test/macros.h>

namespace Karm::Async::Tests {

test$("promise-one-future") {
    Opt<_Future<int>> future;
    {
        _Promise<int> promise;
        future = promise.future();
        promise.resolve(42);
    }
    auto res = run(*future);
    expectEq$(res, 42);
    return Ok();
}

test$("promise-multiple-futures") {
    Opt<_Future<int>> f1, f2, f3;
    {
        _Promise<int> promise;

        f1 = promise.future();
        f2 = promise.future();
        f3 = promise.future();

        promise.resolve(42);
    }

    auto res1 = run(*f1);
    auto res2 = run(*f2);
    auto res3 = run(*f3);

    expectEq$(res1, 42);
    expectEq$(res2, 42);
    expectEq$(res3, 42);

    return Ok();
}

} // namespace Karm::Async::Tests
