#include <karm/entry>

import Mdi;
import Karm.Ui;
import Karm.Kira;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    co_return co_await Ui::runAsync(
        env,
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
