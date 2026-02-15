#include <karm/test>

import Karm.Sys;
import Karm.Logger;

namespace Karm::Sys::Tests {

Async::Task<> bigSleptAsync(Rc<bool> pass, Async::CancellationToken ct) {
    auto res = co_await Sys::globalSched().sleepAsync(Instant::endOfTime(), ct);
    if (not res and res.none().code() == Error::INTERRUPTED)
        *pass = true;
    co_return Ok();
}

testAsync$("async-cancellation") {
    Rc<bool> pass = makeRc<bool>(false);
    bool finished = false;
    Async::Cancellation sleepCancel;
    co_try$(sleepCancel.attach(ct));
    Async::detach(bigSleptAsync(pass, sleepCancel.token()), [&](auto&) {
        finished = true;
    });
    sleepCancel.cancel();

    // wait for the operation to get canceled
    co_trya$(Sys::globalSched().sleepAsync(Sys::instant() + Duration::fromMSecs(16), ct));
    co_expect$(finished);
    co_expect$(pass.unwrap());

    co_return Ok();
}

} // namespace Karm::Sys::Tests
