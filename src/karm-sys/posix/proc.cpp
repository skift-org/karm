module;

#include <fcntl.h>
#include <karm/macros>
#include <signal.h>
#include <sys/wait.h>

export module Karm.Sys.Posix:proc;

import :utils;

using namespace Karm::Literals;
using namespace Karm::Re::Literals;

namespace Karm::Posix {

export struct ProcessStat : Sys::ProcessStat {
    Res<> refresh(Flags<Option> what) override {
        (void)what;
        auto pz = Sys::pageSize();

        auto statusStr = try$(Sys::readAllText("file:/proc"_url / Io::toStr(id) / "status"));
        Io::SScan s{statusStr};

        while (not s.ended()) {
            auto key = s.token(Re::oneOrMore(Re::alnum() | '_'_re));
            s.skip(':');
            s.eat(' '_re | '\t'_re);
            auto value = s.token(Re::until("\n"_re | Re::eof()));
            s.eat(Re::space());

            if (key == "Name"s)
                name = value;
            else if (key == "PPid"s)
                parent = Io::atoi(value).expect();
            else if (key == "State") {
                switch (value[0]) {
                case 'I':
                    status = Sys::ProcessStatus::IDLE;
                    break;
                case 'R':
                    status = Sys::ProcessStatus::RUNNING;
                    break;
                case 'S':
                case 'D':
                    status = Sys::ProcessStatus::SLEEPING;
                    break;

                case 'T':
                    status = Sys::ProcessStatus::STOPPED;
                    break;

                default:
                    status = Sys::ProcessStatus::UNKNOWN;
                    break;
                }
            }
        }

        auto statm = try$(Sys::readAllText("file:/proc"_url / Io::toStr(id) / "statm"));
        s = statm.str();

        (void)Io::atou(s).unwrapOr(0); // size
        s.eat(Re::space());

        memory = Io::atou(s).unwrapOr(0) * pz; // resident
        s.eat(Re::space());
        virtualMemory = Io::atou(s).unwrapOr(0) * pz; // shared

        auto stat = try$(Sys::readAllText("file:/proc"_url / Io::toStr(id) / "stat"));
        s = statm.str();

        s.skip(Re::untilAndConsume(Re::space())); // pid
        s.skip(Re::untilAndConsume(Re::space())); // comm
        s.skip(Re::untilAndConsume(Re::space())); // state
        s.skip(Re::untilAndConsume(Re::space())); // ppid
        s.skip(Re::untilAndConsume(Re::space())); // pgrp
        s.skip(Re::untilAndConsume(Re::space())); // session
        s.skip(Re::untilAndConsume(Re::space())); // tty_nt
        s.skip(Re::untilAndConsume(Re::space())); // tpgid
        s.skip(Re::untilAndConsume(Re::space())); // flags
        s.skip(Re::untilAndConsume(Re::space())); // minflt
        s.skip(Re::untilAndConsume(Re::space())); // cminflt
        s.skip(Re::untilAndConsume(Re::space())); // majflt
        s.skip(Re::untilAndConsume(Re::space())); // cmajflt

        userTime = Sys::Ticks{Io::atou(s).unwrapOr(0)}; // utime
        s.skip(Re::space());
        systemTime = Sys::Ticks{Io::atou(s).unwrapOr(0)}; // stime

        return Ok();
    }
};

export struct Process : Sys::Process {
    pid_t _pid;

    Process(pid_t pid) : Sys::Process(), _pid(pid) {}

    usize id() const override {
        return _pid;
    }

    Res<Rc<Sys::ProcessStat>> stat(Flags<ProcessStat::Option> what) override {
        auto stat = makeRc<ProcessStat>();
        stat->id = _pid;
        try$(stat->refresh(what));
        return Ok(stat);
    }

    Res<> kill() override {
        if (::kill(_pid, SIGKILL) == -1)
            return fromLastErrno();
        return Ok();
    }

    Res<> wait() override {
        int status = 0;
        if (::waitpid(_pid, &status, 0) < 0)
            return Posix::fromLastErrno();
        if (WIFEXITED(status) and WEXITSTATUS(status) == 0)
            return Ok();
        return Error::other("process exited with non-zero status");
    }
};

} // namespace Karm::Posix
