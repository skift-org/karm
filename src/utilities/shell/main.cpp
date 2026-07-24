#include <karm/entry>

import Karm.Cli;
import Karm.Core;
import Karm.Diag;
import Karm.Sh;
import Karm.Sys;
import Karm.Ref;
import Karm.Tty;

using namespace Karm;
using namespace Karm::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto scriptArg = Cli::operand<Str>("script"s, "Script to run"s);

    Cli::Command cmd{
        "shell"s,
        "Scripting language"s,
        {
            {"Input"s, {scriptArg}},
        }
    };

    co_trya$(cmd.execAsync(env));
    if (not cmd)
        co_return Ok();

    Sh::Context context{.pwd = env.cwd()};

    if (scriptArg.value()) {
        auto url = Ref::parseUrlOrPath(scriptArg.value(), env.cwd());
        auto code = co_try$(Sys::readAllText<Utf8>(url));

        Diag::Collector collector;
        auto parseRes = Sh::parse(code, collector);
        if (not parseRes) {
            Diag::Renderer renderer{code, url};
            renderer.render(Sys::err(), collector);
            co_return Error::invalidInput("parser error");
        }

        auto evalRes = parseRes.unwrap().eval();
        if (not evalRes) {
            Sys::errln("runtime error {}: {}", scriptArg.value(), evalRes);
            co_return Error::invalidInput("runtime error");
        }

        co_return Ok();
    }

    while (true) {
        Sys::print("{} {} ", context.pwd | Tty::RESET, "λ"s | Tty::Style{.foreground = Tty::BLUE});
        auto line = co_try$(Io::readLine<Utf8>(Sys::in()));

        Diag::Collector collector;
        auto parseRes = Sh::parse(line, collector);
        if (not parseRes) {
            Diag::Renderer renderer{line, String{"<line>"}};
            renderer.render(Sys::err(), collector);
            continue;
        }

        auto evalRes = parseRes.unwrap().eval();
        if (not evalRes)
            Sys::errln("runtime error {}: {}", scriptArg.value(), evalRes);
    }

    co_return Ok();
}
