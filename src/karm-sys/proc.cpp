module;

#include <karm-core/macros.h>

export module Karm.Sys:proc;

import :_embed;
import :time;
import :pid;
import :sandbox;

namespace Karm::Sys {

export Res<> sleep(Duration span) {
    return _Embed::sleep(span);
}

export Res<> sleepUntil(Instant until) {
    return _Embed::sleepUntil(until);
}

export Res<Ref::Url> pwd() {
    return _Embed::pwd();
}

export [[noreturn]] void exit(Res<> res) {
    _Embed::exit(res ? 0 : -toUnderlyingType(res.none().code()))
        .unwrap();
    unreachable();
}

// MARK: Process ---------------------------------------------------------------

export struct Process {
    Rc<Pid> _pid;

    Res<> kill() { return _pid->kill(); }

    Res<> wait() { return _pid->wait(); }
};

export struct Command {
    String exe;
    Vec<String> args = {};
    Map<String, String> env = {};
    Opt<Rc<Fd>> in = NONE, out = NONE, err = NONE;

    Res<Process> run() {
        auto pid = try$(_Embed::run(*this));
        return Ok(pid);
    }
};

} // namespace Karm::Sys
