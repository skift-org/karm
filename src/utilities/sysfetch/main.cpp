#include <karm/entry>

import Karm.Tty;
import Karm.App;
import Karm.Core;
import Karm.Sys;

using namespace Karm;

static Str const ART = R"(
        ___
       /\  \
      /::\  \
     /:/\ \  \
    _\:\~\ \  \
   /\ \:\ \ \__\
   \:\ \:\ \/__/
    \:\ \:\__\
     \:\/:/  /
      \::/  /
       \/__/
)";

static Str const BLOCK = "███";

namespace Sysfetch {

auto title(Str text) {
    return Io::format("{}{}", Tty::Cmd::forward(19), text | Tty::Style{.bold = true});
}

Res<> dumpUserInfo() {
    auto userinfo = try$(Sys::userinfo());
    Sys::println("{}: {} {}", title("User"), userinfo.name);
    Sys::println("{}: {} {}", title("Home"), userinfo.home);
    Sys::println("{}: {} {}", title("Shell"), userinfo.shell);
    return Ok();
}

Res<> dumpSysInfo() {
    auto sysinfo = try$(Sys::sysinfo());
    Sys::println("{}: {} {}", title("System"), sysinfo.sysName, sysinfo.sysVersion);
    Sys::println("{}: {} {}", title("Kernel"), sysinfo.kernelName, sysinfo.kernelVersion);
    Sys::println("{}: {}", title("Hostname"), sysinfo.hostname);
    Sys::println("{}: {}", title("Form factor"), App::formFactor == App::FormFactor::DESKTOP ? "🖥️" : "📱");
    return Ok();
}

Res<> dumpMemInfo() {
    auto meminfo = try$(Sys::meminfo());
    Sys::println("{}: {} / {}", title("Memory"), meminfo.physicalUsed, meminfo.physicalTotal);
    Sys::println("{}: {} / {}", title("Swap"), meminfo.swapUsed, meminfo.swapTotal);
    return Ok();
}

Res<> dumpCpusInfo() {
    auto cpusinfo = try$(Sys::cpusinfo());

    for (auto& cpu : cpusinfo) {
        Sys::println("{}: {} {} {} MHz", title("CPU"), cpu.name, cpu.brand, cpu.freq);
    }

    return Ok();
}

Res<> testAnsi() {
    Sys::print("{}", Tty::Cmd::forward(19));
    for (auto const c : Tty::DARK_COLORS) {
        Sys::print("{}", BLOCK | Tty::Style{.foreground = c});
    }

    Sys::println("");

    Sys::print("{}", Tty::Cmd::forward(19));
    for (auto const c : Tty::LIGHT_COLORS) {
        Sys::print("{}", BLOCK | Tty::Style{.foreground = c});
    }

    Sys::println("");

    return Ok();
}

Res<> dumpKindness() {
    Sys::println("");
    Sys::print("{}", Tty::Cmd::forward(19));
    Sys::println("🏳️‍🌈{}", wholesome(Sys::now().val()));

    return Ok();
}

} // namespace Sysfetch

Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    Res<> res = Ok();

    Sys::println("{}", ART | Tty::Style{.foreground = Tty::BLUE});
    Sys::print("{}", Tty::Cmd::up(12));

    if (not(res = Sysfetch::dumpUserInfo())) {
        Sys::errln("{}: {}", Sysfetch::title("User"), res.none().msg() | Tty::Style{.foreground = Tty::RED_LIGHT});
    }

    if (not(res = Sysfetch::dumpSysInfo())) {
        Sys::errln("{} {}", Sysfetch::title("System"), res.none().msg() | Tty::Style{.foreground = Tty::RED_LIGHT});
    }

    if (not(res = Sysfetch::dumpMemInfo())) {
        Sys::errln("{}: {}", Sysfetch::title("Memory"), res.none().msg() | Tty::Style{.foreground = Tty::RED_LIGHT});
    }

    if (not(res = Sysfetch::dumpCpusInfo())) {
        Sys::errln("{}: {}", Sysfetch::title("CPUs"), res.none().msg() | Tty::Style{.foreground = Tty::RED_LIGHT});
    }

    if (not(res = Sysfetch::dumpKindness())) {
        Sys::errln("{}: {}", Sysfetch::title("Kindness"), res.none().msg() | Tty::Style{.foreground = Tty::RED_LIGHT});
    }

    Sys::println("");
    res = Sysfetch::testAnsi();
    Sys::println("");

    co_return res;
}
