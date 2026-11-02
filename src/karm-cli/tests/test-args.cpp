#include <karm/test>

import Karm.Cli;
import Karm.Sys;

using namespace Karm::Literals;

namespace Karm::Cli::Tests {

test$("args-tokenizer") {
    Array args = {
        "ls"s,
        "-h"s,
        "--help"s,
        "-la"s,
        "/usr/bin"s,
    };

    Vec<Token> tokens;
    tokenize(args, tokens);

    assertEq$(tokens.len(), 6uz);

    assertEq$(tokens[0].kind, Token::OPERAND);
    assertEq$(tokens[0].value, "ls"s);

    assertEq$(tokens[1].kind, Token::FLAG);
    assertEq$(tokens[1].flag, (Rune)'h');

    assertEq$(tokens[2].kind, Token::OPTION);
    assertEq$(tokens[2].value, "help"s);

    assertEq$(tokens[3].kind, Token::FLAG);
    assertEq$(tokens[3].flag, (Rune)'l');

    assertEq$(tokens[4].kind, Token::FLAG);
    assertEq$(tokens[4].flag, (Rune)'a');

    assertEq$(tokens[5].kind, Token::OPERAND);
    assertEq$(tokens[5].value, "/usr/bin"s);

    return Ok();
}

testAsync$("args-simple-command") {
    Command cmd{
        "test"s,
    };

    Vec<Str> args = {};
    co_try$(cmd.exec(args));

    if (not cmd)
        co_return Error::other("command not invoked");

    co_return Ok();
}

testAsync$("args-nested-command") {
    Command cmd{
        "test"s,
    };

    auto& subCmd = cmd.subCommand("sub"s);

    Array<Str, 1> args = {"sub"s};
    co_try$(cmd.exec(args));

    if (not cmd)
        co_return Error::other("command not invoked");

    if (not subCmd)
        co_return Error::other("sub-command not invoked");

    co_return Ok();
}

testAsync$("args-partial-command") {
    auto headerArg = option<Opt<Str>>(NONE, "header"s, ""s, NONE);

    Command cmd{
        "test"s,
        ""s,
        {{.title = ""s, .options = {headerArg}}},
        true,
    };

    Array args = {"--header"s, "h"s, "-o"s, "out"s};
    Vec<Token> tokens;
    tokenize(args, tokens);
    Cursor<Token> c = tokens;
    co_try$(cmd.exec(c));

    if (not cmd)
        co_return Error::other("command not invoked");

    co_assertEq$(headerArg.value(), Opt<Str>{Some("h"s)});
    co_assertEq$(c.rem(), 2uz);
    co_assertEq$(c->flag, (Rune)'o');

    co_return Ok();
}

} // namespace Karm::Cli::Tests
