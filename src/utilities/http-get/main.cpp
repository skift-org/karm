#include <karm/entry>

import Karm.Cli;
import Karm.Http;
import Karm.Core;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto urlArg = Cli::operand<Str>("url"s, "URL to fetch"s, "localhost"s);

    Cli::Command cmd{
        "http-get"s,
        "Send a GET request to a URL and print the response body"s,
        {
            {
                "Request Options"s,
                {
                    urlArg,
                },
            },
        }
    };

    co_trya$(cmd.execAsync(env));

    auto url = Ref::parseUrlOrPath(urlArg.value(), env.cwd());
    auto resp = co_trya$(Http::getAsync(url, ct));
    if (not resp->body)
        co_return Error::invalidData("no body in response");

    auto body = resp->body.take();

    auto adaptedOut = Aio::adapt(Sys::out());
    co_trya$(Aio::copyAsync(*body, adaptedOut, ct));

    co_return Ok();
}
