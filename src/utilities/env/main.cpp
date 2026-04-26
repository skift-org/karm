#include <karm/entry>

import Karm.Cli;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    Cli::Command cmd{
        "env"s,
        "Do things with environement variables."s,
    };

    Cli::Command& dump = cmd.subCommand(
        "dump"s,
        "Dump the environment"s
    );

    auto name = Cli::operand<Str>("name"s, "Name of the variable"s);

    Cli::Command& get = cmd.subCommand(
        "get"s,
        "Get an environment variable value"s,
        {{
            "Arguments"s,
            {
                name,
            },
        }}
    );

    co_trya$(cmd.execAsync(env));

    if (dump) {
        for (auto& e : env.vars().iter()) {
            Sys::println("{}={}"s, e.v0, e.v1);
        }
    } else if (get) {
        if (not env.vars().has(name.value()))
            co_return Error::invalidInput("This environment variable does not exist."s);
        Sys::println("{}"s, env.vars().get(name.value()));
    }

    co_return Ok();
}
