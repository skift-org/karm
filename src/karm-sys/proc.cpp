module;

#include <karm/macros>

export module Karm.Sys:proc;

import :_embed;
import :time;
import :pid;
import :sandbox;
import :pty;

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

    Res<Process> spawn() {
        auto pid = try$(_Embed::spawn(*this));
        return Ok(pid);
    }

    Res<Tuple<Process, Pty>> spawnPty() {
        auto [pid, fd] = try$(_Embed::spawnPty(*this));
        return Ok<Tuple<Process, Pty>>(Process{pid}, Pty{fd});
    }
};

} // namespace Karm::Sys
