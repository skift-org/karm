#include <karm/entry>

import Karm.Cli;
import Karm.Md;
import Karm.Ref;
import Karm.Sys;

using namespace Karm;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto urlArg = Cli::operand<Ref::Url>("url"s, "Markdown document to convert to HTML"s);

    Cli::Command cmd{
        "md2html"s,
        "Convert a Markdown document to HTML using karm-md."s,
        {{
            "Arguments"s,
            {
                urlArg,
            },
        }},
    };

    co_trya$(cmd.execAsync(env));

    if (cmd) {
        auto markdown = co_try$(Sys::readAllUtf8(urlArg.value()));
        auto document = Md::parse(markdown);
        auto html = Md::renderHtml(document);

        co_try$(Sys::out().writeStr(html.str()));
        co_try$(Sys::out().flush());
    }

    co_return Ok();
}
