export module Karm.Test:driver;

import Karm.Core;
import Karm.Sys;
import Karm.Logger;
import Karm.Tty;
import Karm.Glob;

import :test;

using namespace Karm::Literals;

namespace Karm::Test {

static constexpr Tty::Style TTY_PASS = {.foreground = Tty::GREEN, .bold = true};
static constexpr Tty::Style TTY_FAIL = {.foreground = Tty::RED, .bold = true};
static constexpr Tty::Style TTY_XFAIL = {.foreground = Tty::CYAN, .bold = true};
static constexpr Tty::Style TTY_XPASS = {.foreground = Tty::MAGENTA, .bold = true};
static constexpr Tty::Style TTY_WARN = {.foreground = Tty::YELLOW, .bold = true};
static constexpr Tty::Style TTY_NOTE = {.foreground = Tty::GRAY_DARK, .bold = true};

export struct RunOptions {
    String glob = "*"s;
    bool fast = false;
};

export struct Driver {
    Async::Task<> runAllAsync(RunOptions options, Async::CancellationToken ct) {
        usize passed = 0, failed = 0, skipped = 0, xFailed = 0, xPassed = 0;

        Sys::errln("Running {} tests…", Test::len());
        if (options.glob != "*")
            Sys::errln("Matching glob: {:#}", options.glob);

        Sys::errln("");

        for (auto* test = Test::first(); test; test = test->next) {
            if (not Glob::matchGlob(options.glob, test->name))
                continue;

            Sys::err("Running {}:{}: {}… ", test->sourceLocation.file | Tty::WHITE, test->sourceLocation.line, test->name | Tty::BLUE_LIGHT);

            auto result = co_await test->runAsync(*this, ct);

            if (not result and result.none() == Error::SKIPPED) {
                skipped++;
                Sys::errln("{}", "SKIP"s | TTY_WARN);
            } else if (test->xFail) {
                if (result) {
                    if (options.fast) {
                        Sys::errln("{}", "XPASS"s | TTY_XPASS);
                        Sys::errln("{}", test->sourceLocation);
                        co_return Error::other("expected failure test passed");
                    }
                    xPassed++;
                    Sys::errln("{}", "XPASS"s | TTY_XPASS);
                } else {
                    xFailed++;
                    Sys::errln("{}", "XFAIL"s | TTY_XFAIL);
                }
            } else if (not result) {
                if (options.fast) {
                    Sys::errln("{}", "FAIL"s | TTY_FAIL);
                    Sys::errln("{}", test->sourceLocation);
                    co_return Error::other("test failed");
                }
                failed++;
                Sys::errln("{}", Io::cased(result, Io::Case::UPPER) | TTY_FAIL);
            } else {
                passed++;
                Sys::errln("{}", "PASS"s | TTY_PASS);
            }
        }

        Sys::errln("");

        if (skipped) {
            Sys::errln(
                " {:5} skipped",
                skipped | TTY_WARN
            );
        }

        if (xFailed) {
            Sys::errln(
                " {:5} failed as expected",
                xFailed | TTY_XFAIL
            );
        }

        if (failed or xPassed) {
            if (failed) {
                Sys::errln(
                    " {:5} failed - {} {}",
                    failed | TTY_FAIL,
                    witty(Sys::now().val()) | TTY_NOTE,
                    badEmoji(Sys::now().val())
                );
            }

            if (xPassed) {
                Sys::errln(
                    " {:5} fixed - remove XFAIL from their test$ {}",
                    xPassed | TTY_XPASS,
                    goodEmoji(Sys::now().val())
                );
            }

            Sys::errln(
                " {:5} passed\n",
                passed | TTY_PASS
            );

            co_return Error::other("test failed");
        }

        Sys::errln(
            " {:5} passed - {} {}\n",
            passed | TTY_PASS,
            nice(Sys::now().val()) | TTY_NOTE,
            goodEmoji(Sys::now().val())
        );

        co_return Ok();
    }

    Res<> unexpect(auto const& lhs, auto const& rhs, Str op, SourceLocation sourceLocation = SourceLocation::current()) {
        logError({"unexpected: {:#} {} {:#}"s, sourceLocation}, lhs, op, rhs);
        return Error::other("unexpected");
    }
};

} // namespace Karm::Test
