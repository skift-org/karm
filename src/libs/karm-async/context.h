#pragma once

#include <karm-base/time.h>

#include "cancelation.h"

namespace Karm::Async {

struct Context {
    Opt<Async::Cancelation::Token> _token;
    Instant _deadline = Instant::END_OF_TIME;

    static Context background() {
        return Context{};
    }

    Context withDeadline(Instant deadline) {
        auto copy = *this;
        copy._deadline = deadline;
        return copy;
    }

    Res<> errorIfCanceled() {
        if (_token)
            return _token->errorIfCanceled();
        return Ok();
    }

    Tuple<Context, Rc<Cancelation>> withCancel() {
        auto copy = *this;
        auto [cancelation, token] = Cancelation::create();
        copy._token = token;
        return {copy, cancelation};
    }
};

} // namespace Karm::Async
