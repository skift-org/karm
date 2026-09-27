import Karm.Core;

#include <karm/test>

namespace Karm::Async::Tests {

test$("promise-one-future") {
    Opt<Async::_Future<int>> future;
    {
        Async::_Promise<int> promise;
        future = Some(promise.future());
        promise.resolve(42);
    }
    auto res = Async::run(*future);
    assertEq$(res, 42);
    return Ok();
}

test$("promise-multiple-futures") {
    Opt<Async::_Future<int>> f1, f2, f3;
    {
        Async::_Promise<int> promise;

        f1 = Some(promise.future());
        f2 = Some(promise.future());
        f3 = Some(promise.future());

        promise.resolve(42);
    }

    auto res1 = Async::run(*f1);
    auto res2 = Async::run(*f2);
    auto res3 = Async::run(*f3);

    assertEq$(res1, 42);
    assertEq$(res2, 42);
    assertEq$(res3, 42);

    return Ok();
}

} // namespace Karm::Async::Tests
