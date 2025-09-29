#pragma once

import Karm.Sys;
import Karm.Core;

namespace Karm::Posix {

struct Fd : Sys::Fd {
    isize _raw;
    bool _leak = false; //< Do not close on destruction

    Fd(isize raw);

    ~Fd() override;

    Res<usize> read(MutBytes bytes) override;

    Res<usize> write(Bytes bytes) override;

    Res<usize> seek(Io::Seek seek) override;

    Res<> flush() override;

    Res<Rc<Sys::Fd>> dup() override;

    Res<Sys::_Accepted> accept() override;

    Res<Sys::Stat> stat() override;

    Res<Sys::_Sent> send(Bytes, Slice<Sys::Handle>, Sys::SocketAddr) override;

    Res<Sys::_Received> recv(MutBytes, MutSlice<Sys::Handle>) override;
};

static inline Res<Rc<Posix::Fd>> toPosixFd(Rc<Sys::Fd> fd) {
    auto posixFd = fd.cast<Posix::Fd>();
    if (not posixFd)
        return Error::invalidInput("expected posix fd");
    return Ok(posixFd.take());
}

} // namespace Karm::Posix
