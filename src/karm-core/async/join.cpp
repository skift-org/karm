export module Karm.Core:async.join;

import :async.base;
import :base.tuple;
import :base.opt;
import :meta.nocopy;
import :async.promise;
import :async.run;

namespace Karm::Async {

export template <typename... Ss>
auto join(Ss... senders) {
    struct State {
        usize pending;
        Promise<> promise{};
    };

    auto* state = new State{sizeof...(Ss)};
    auto future = state->promise.future();
    auto onComplete = [state](auto) {
        --state->pending;
        if (state->pending == 0) {
            state->promise.resolve(Ok());
            delete state;
        }
    };
    (Async::detach(std::move(senders), onComplete), ...);
    return future;
}

export template <typename... Ss>
auto any(Ss... senders) {
    struct State {
        bool done = false;
        Promise<> promise{};
    };

    auto* state = new State{};
    auto future = state->promise.future();
    auto onComplete = [state](auto) {
        if (state->done)
            return;
        state->done = true;
        state->promise.resolve(Ok());
        delete state;
    };
    (Async::detach(std::move(senders), onComplete), ...);
    return future;
}

} // namespace Karm::Async
