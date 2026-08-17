#include <karm/entry>

import Karm.Cli;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto source = Cli::operand<Ref::Url>("source"s, "Path of the file or directory to rename"s);
    auto destination = Cli::operand<Ref::Url>("destination"s, "New path for the file or directory"s);

    Cli::Command cmd{
        "rename"s,
        "Rename or move a file or directory."s,
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

    co_return Sys::rename(source, destination);
}
