#include <karm/entry>

import Karm.Ui;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context& ctx, Async::CancellationToken ct) {
    co_return co_await Ui::runAsync(
        ctx,
        Ui::button(Ui::SINK<>, "Hello, world"s) |
            Ui::center() |
            Ui::pinSize({300, 300}),
        ct
    );
}
