#include <karm/entry>

import Karm.Cli;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto file = Cli::operand<Ref::Url>("file"s, "Path of the file to create or update"s);

    Cli::Command cmd{
        "touch"s,
        "Create a file, or update its access and modification timestamps."s,
        {
            {
                "Options"s,
                {
                    file,
                },
            },
        }
    };
    co_try$(cmd.exec(env));
    if (not cmd)
        co_return Ok();

    co_return Sys::touch(file);
}
