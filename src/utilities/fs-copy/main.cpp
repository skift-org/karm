#include <karm/entry>

import Karm.Cli;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto source = Cli::operand<Ref::Url>("source"s, "Path of the file to copy"s);
    auto destination = Cli::operand<Ref::Url>("destination"s, "New path for the file"s);

    Cli::Command cmd{
        "copy"s,
        "copy a file."s,
        {
            {
                "Options"s,
                {
                    source,
                    destination,
                },
            },
        }
    };

    co_try$(cmd.exec(env));
    if (not cmd)
        co_return Ok();

    co_try$(Sys::copy(source, destination));
    co_return Ok();
}
