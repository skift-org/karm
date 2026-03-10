#include <karm/entry>

import Karm.Cli;
import Karm.Ref;
import Karm.Sys;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Context& ctx, Async::CancellationToken) {
    auto urlArg = Cli::operand<Ref::Url>("url"s, "First URL or path to sniff from file contents"s);
    auto restArg = Cli::extra("Additional URLs or paths to sniff from file contents"s);

    Cli::Command cmd{
        "uti-sniff-data"s,
        "Resolve UTIs by sniffing the contents of one or more URLs."s,
        {{
            "Arguments"s,
            {
                urlArg,
                restArg,
            },
        }},
    };

    co_trya$(cmd.execAsync(ctx));

    if (cmd) {
        auto dump = [&](Ref::Url const& url) -> Async::Task<> {
            auto file = co_try$(Sys::File::open(url));
            Sys::println("{}", file.sniff());
            co_return Ok();
        };

        co_trya$(dump(urlArg.value()));

        auto pwd = co_try$(Sys::pwd());
        for (auto const& arg : restArg.value()) {
            auto url = Ref::parseUrlOrPath(arg, pwd);
            co_trya$(dump(url));
        }
    }

    co_return Ok();
}
