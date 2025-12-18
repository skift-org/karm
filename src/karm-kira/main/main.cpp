#include <karm-sys/entry.h>

import Mdi;
import Karm.Ui;
import Karm.Kira;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context& ctx, Async::CancellationToken ct) {
    co_return co_await Ui::runAsync(
        ctx,
        Kr::scaffold({
            .icon = Mdi::DUCK,
            .title = "Kira Application"s,
            .body = [] {
                return Ui::labelMedium("Hello, world"s);
            },
        }),
        ct
    );
}
