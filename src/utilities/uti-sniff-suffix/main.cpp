#include <karm/entry>

import Karm.Cli;
import Karm.Ref;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto urlArg = Cli::operand<Ref::Url>("url"s, "First URL or path to resolve by suffix"s);
    auto restArg = Cli::extra("Additional URLs or paths to resolve by suffix"s);

    Cli::Command cmd{
        "uti-sniff-suffix"s,
        "Resolve UTIs from the path suffix of one or more URLs."s,
        {{
            "Arguments"s,
            {
                urlArg,
                restArg,
            },
        }},
    };

    co_trya$(cmd.execAsync(env));

    if (cmd) {
        auto dump = [](Ref::Url const& url) {
            Sys::println("{}", Ref::Uti::fromSuffix(url.path.suffix()));
        };

        dump(urlArg.value());

        for (auto const& arg : restArg.value()) {
            auto url = Ref::parseUrlOrPath(arg, env.cwd());
            dump(url);
        }
    }

    co_return Ok();
}
