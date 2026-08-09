#include <karm/entry>

import Karm.Ui;
import Karm.Signals;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    Signals::Signal count = 0;
    Signals::Computed count2 = [=] {
        return count.value() * 2;
    };

    auto app = Ui::reactive(
        [=] {
            return Ui::vflow(
                Ui::text("Counter: {}", count2.value()),
                Ui::button(
                    Some([count](Ui::Node&) mutable {
                        count.update(count.peek() + 1);
                    }),
                    "Increment"
                )
            );
        }
    );

    co_return co_await Ui::runAsync(
        env,
        app,
        ct
    );
}
