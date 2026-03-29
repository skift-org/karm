#include <karm/entry>

import Karm.Cli;
import Karm.Ref;
import Karm.Sys;

using namespace Karm;

static void _dumpRegistration(Ref::Uti uti) {
    Sys::println("name: {}", uti.name());
    if (uti.description().len() == 0)
        Sys::println("description: <none>");
    else
        Sys::println("description: {}", uti.description());

    if (auto suffix = uti.primarySuffix(); suffix)
        Sys::println("primary suffix: {}", suffix.unwrap());
    else
        Sys::println("primary suffix: <none>");

    Sys::println("primary mime: {}", uti.primaryMimeType());
    Sys::println("suffixes: {}", uti.suffixes());
    Sys::println("mime types: {}", uti.mimeTypes());
    Sys::println("conforms to: {}", uti.declaredConformances());
}

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto queryArg = Cli::operand<Str>("query"s, "UTI name, suffix, or MIME type to inspect"s);

    Cli::Command cmd{
        "uti-dump"s,
        "Dump the registration details for a UTI resolved from a name, suffix, or MIME type."s,
        {{
            "Arguments"s,
            {
                queryArg,
            },
        }},
    };

    co_trya$(cmd.execAsync(env));

    if (cmd) {
        auto query = queryArg.value();
        Ref::Uti uti = (contains(query, "/"s) or contains(query, "."s))
                           ? Ref::Uti::fromUtiOrMime(query)
                           : Ref::Uti::fromSuffix(query);

        Sys::println("query: {}", query);
        _dumpRegistration(uti);
    }

    co_return Ok();
}
