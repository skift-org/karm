module;

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

export module Karm.Sys.Posix:fd;

import Karm.Sys;
import Karm.Core;
import :utils;

namespace Karm::Posix {

export struct Fd : Sys::Fd {
    isize _raw;
    bool _leak = false; //< Do not close on destruction

    Fd(isize raw) : _raw(raw) {}

    ~Fd() override {
        if (not _leak)
            close(_raw);
    }

    Res<usize> read(MutBytes bytes) override {
        isize result = ::read(_raw, bytes.buf(), sizeOf(bytes));

        if (result < 0)
            return fromLastErrno();

        return Ok(static_cast<usize>(result));
    }

    Res<usize> write(Bytes bytes) override {
        isize result = ::write(_raw, bytes.buf(), sizeOf(bytes));

        if (result < 0)
            return fromLastErrno();

        return Ok(static_cast<usize>(result));
    }

    Res<usize> seek(Io::Seek seek) override {
        off_t offset = 0;

        switch (seek.whence) {
        case Io::Whence::BEGIN:
            offset = lseek(_raw, seek.offset, SEEK_SET);
            break;
        case Io::Whence::CURRENT:
            offset = lseek(_raw, seek.offset, SEEK_CUR);
            break;
        case Io::Whence::END:
            offset = lseek(_raw, seek.offset, SEEK_END);
            break;
        }

        if (offset < 0)
            return fromLastErrno();

        return Ok(static_cast<usize>(offset));
    }

    Res<> flush() override {
        // NOTE: No-op
        return Ok();
    }

    Res<Rc<Sys::Fd>> dup() override {
        isize duped = ::dup(_raw);

        if (duped < 0)
            return fromLastErrno();

        return Ok(makeRc<Fd>(duped));
    }

    Res<Sys::_Accepted> accept() override {
        sockaddr_in addr_;
        socklen_t len = sizeof(addr_);
        isize fd = ::accept(_raw, (struct sockaddr*)&addr_, &len);
        if (fd < 0)
            return fromLastErrno();

        return Ok<Sys::_Accepted>(
            makeRc<Fd>(fd),
            fromSockAddr(addr_)
        );
    }

    Res<Sys::Stat> stat() override {
        struct stat buf;
        if (fstat(_raw, &buf) < 0)
            return fromLastErrno();
        return Ok(fromStat(buf));
    }

    Res<Sys::_Sent> send(Bytes bytes, Slice<Sys::Handle> handles, Sys::SocketAddr addr) override {
        if (handles.len() > 0)
            // TODO: Implement handle passing on POSIX
            notImplemented();

        sockaddr_in addr_ = toSockAddr(addr);
        isize result = sendto(
            _raw,

            bytes.buf(),
            bytes.len(),

            0,
            reinterpret_cast<struct sockaddr*>(&addr_),
            sizeof(addr_)
        );

        if (result < 0)
            return fromLastErrno();

        return Ok<Sys::_Sent>(static_cast<usize>(result), 0);
    }

    Res<Sys::_Received> recv(MutBytes bytes, MutSlice<Sys::Handle> handles) override {
        (void)handles;

        sockaddr_in addr_{};
        socklen_t len = sizeof(addr_);
        isize result = recvfrom(
            _raw,

            bytes.buf(),
            bytes.len(),

            0,
            reinterpret_cast<struct sockaddr*>(&addr_), &len
        );

        if (result < 0)
            return fromLastErrno();

        return Ok<Sys::_Received>(
            static_cast<usize>(result),
            0,
            fromSockAddr(addr_)
        );
    }
};

export Res<Rc<Fd>> ensurePosixFd(Rc<Sys::Fd> fd) {
    auto posixFd = fd.cast<Fd>();
    if (not posixFd)
        return Error::invalidInput("expected posix fd");
    return Ok(posixFd.take());
}

} // namespace Karm::Posix
