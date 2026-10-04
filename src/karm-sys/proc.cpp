module;

#include <karm/macros>

export module Karm.Sys:proc;

import Karm.Sys.Base;
import :_embed;
import :time;
import :pty;

using namespace Karm::Literals;

namespace Karm::Sys {

export Res<> sleep(Duration span) {
    return _Embed::sleep(span);
}

export Res<> sleepUntil(Instant until) {
    return _Embed::sleepUntil(until);
}

export [[noreturn]] void exit(Res<> res) {
    _Embed::exit(res ? 0 : -toUnderlyingType(res.none().code()))
        .expect();
    unreachable();
}

// MARK: Process ---------------------------------------------------------------

export enum struct ProcessStatus {
    UNKNOWN,
    IDLE,
    RUNNING,
    SLEEPING,
    STOPPED,

    _LEN,
};

export struct ProcessStat {
    enum struct Option : u16 {
        NAME = 1 << 0,
        COMMAND = 1 << 1,
        EXECUTABLE = 1 << 2,
        ENVIRON = 1 << 3,
        CWD = 1 << 4,
        PARENT = 1 << 5,
        MEMORY = 1 << 6,
        VIRTUAL_MEMORY = 1 << 7,
        STATUS = 1 << 8,
        USER_TIME = 1 << 9,
        SYSTEM_TIME = 1 << 10,

        ALL = 0xffff,
    };

    using enum Option;

    usize id;
    String name;
    Vec<String> command;
    String executable;
    Vec<String> environ;
    Ref::Url cwd;
    usize parent;
    usize memory;
    usize virtualMemory;
    ProcessStatus status;
    Ticks userTime;
    Ticks systemTime;

    virtual ~ProcessStat() = default;

    virtual Res<> refresh(Flags<Option> what) = 0;
};

export struct Process {
    static Res<Vec<Rc<Process>>> list() {
        return _Embed::listProcess();
    }

    virtual usize id() const = 0;

    virtual ~Process() = default;

    virtual Res<Rc<ProcessStat>> stat(Flags<ProcessStat::Option> what) = 0;

    virtual Res<> kill() = 0;

    virtual Res<> wait() = 0;
};

export struct Command {
    String exe = ""s;
    Vec<String> args = {};
    Map<String, String> env = {};
    Opt<Rc<Fd>> in = NONE, out = NONE, err = NONE;

    Res<Rc<Process>> spawn() {
        auto pid = try$(_Embed::spawn(*this));
        return Ok(pid);
    }

    Res<Tuple<Rc<Process>, Pty>> spawnPty() {
        auto [pid, fd] = try$(_Embed::spawnPty(*this));
        return Ok<Tuple<Rc<Process>, Pty>>(pid, Pty{fd});
    }
};

} // namespace Karm::Sys
