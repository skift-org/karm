#include <karm/entry>

import Karm.Cli;
import Karm.Test;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context& ctx, Async::CancellationToken ct) {
    auto globArg = Cli::option<Str>(
        'g',
        "glob"s,
        "Run tests matching this glob pattern"s,
        "*"s
    );
    auto fastArg = Cli::flag(
        'f',
        "fast"s,
        "Stop on first failure"s
    );

    Cli::Command cmd{
        "karm-test"s,
        "Run the Karm test suite"s,
        {{
            "Test Options"s,
            {
                globArg,
                fastArg,
            },
        }},
        [=](Sys::Context&) -> Async::Task<> {
            Test::Driver driver;
            co_return co_await driver.runAllAsync(
                {
                    .glob = globArg.value(),
                    .fast = fastArg.value(),
                },
                ct
            );
        },
    };

    co_return co_await cmd.execAsync(ctx);
}
