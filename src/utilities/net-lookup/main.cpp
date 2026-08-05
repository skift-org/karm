#include <karm/entry>

import Karm.Cli;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto domainArg = Cli::operand<Str>("domain"s, "Domain name to lookup"s);

    Cli::Command cmd{
        "net-lookup"s,
        "Do a domain name lookup"s,
        {
            {
                "Lookup Options"s,
                {domainArg},
            },
        }
    };

    co_try$(cmd.exec(env));

    auto ips = co_trya$(Sys::lookupAsync(domainArg.value()));
    co_try$(Io::format(Sys::out(), "{}:\n", domainArg.value()));
    for (auto& ip : ips) {
        co_try$(Io::format(Sys::out(), "{}\n", ip));
    }

    co_return Ok();
}
