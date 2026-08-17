#include <karm/entry>

import Karm.Cli;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto path = Cli::operand<Ref::Url>("path"s, "Path of the file or directory to remove"s);
    auto fileFlag = Cli::flag(Some('f'), "file"s, "Remove regular files"s);
    auto directoryFlag = Cli::flag(Some('d'), "directory"s, "Remove directories"s);
    auto recursiveFlag = Cli::flag(Some('r'), "recursive"s, "Remove directories and their contents recursively"s);

    Cli::Command cmd{
        "remove"s,
        "Remove files or directories."s,
        {
            {
                "Options"s,
                {
                    path,
                    fileFlag,
                    directoryFlag,
                    recursiveFlag,
                },
            },
        }
    };

    co_try$(cmd.exec(env));
    if (not cmd)
        co_return Ok();

    Flags<Sys::RemoveOption> options = {};
    if ((not fileFlag and not directoryFlag) or fileFlag)
        options.set(Sys::RemoveOption::FILE);

    if (directoryFlag)
        options.set(Sys::RemoveOption::DIRECTORY);

    if (recursiveFlag)
        options.set(Sys::RemoveOption::RECURSIVE);

    co_return Sys::remove(path, options);
}
