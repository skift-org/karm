#include <karm/entry>

import Karm.Ui;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    co_return co_await Ui::runAsync(
        env,
        Ui::button(Ui::SINK<>, "Hello, world"s) |
            Ui::center() |
            Ui::pinSize({300, 300}),
        ct
    );
}
