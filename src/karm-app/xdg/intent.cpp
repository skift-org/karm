module;

#include <sys/wait.h>
#include <unistd.h>
//
#include <karm/macros>

module Karm.App;

import Karm.Sys.Posix;

namespace Karm::App::_Embed {

Res<> launch(Intent const& intent) {
    if (intent.attachments.len() != 1)
        return Error::invalidInput("expected exactly one object");

    auto [url, _] = intent.attachments[0];

    String str = ""s;
    if (url.scheme == "file"s or url.scheme == "bundle"s) {
        str = try$(Posix::resolve(url)).str();
    } else {
        str = url.str();
    }

    int pid = fork();
    if (pid < 0)
        return Posix::fromLastErrno();

    if (pid == 0) {
#ifdef __ck_sys_darwin__
        if (intent.action == Ref::Uti::PUBLIC_MODIFY) {
            execlp("open", "open", "-t", str.buf(), nullptr);
        } else {
            execlp("open", "open", str.buf(), nullptr);
        }
#else
        execlp("xdg-open", "xdg-open", str.buf(), nullptr);
#endif
        _exit(1);
    }

    auto status = 0;
    if (waitpid(pid, &status, 0) < 0)
        return Posix::fromLastErrno();

    if (WIFEXITED(status) and WEXITSTATUS(status) == 0)
        return Ok();

    return Posix::fromStatus(status);
}

Async::Task<> launchAsync(Intent intent) {
    co_return _Embed::launch(intent);
}

} // namespace Karm::App::_Embed