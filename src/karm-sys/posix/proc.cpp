module;

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

export module Karm.Sys.Posix:proc;

import :utils;

namespace Karm::Posix {

export struct Pid : Sys::Pid {
    pid_t _pid;

    Pid(pid_t pid) : Sys::Pid(), _pid(pid) {}

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
