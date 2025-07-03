#include "win32.h"

#include "fd.h"
#include "utils.h"

namespace Win32 {

Fd::Fd(isize raw) : _raw(raw) {}

Fd::~Fd() {
    if (not _leak)
        if (::CloseHandle(_handle) == 0)
            return Win32::fromGetLastError();
}

Sys::Handle Fd::handle() const {
    return Sys::Handle{static_cast<usize>(_handle)};
}

Res<usize> Fd::read(MutBytes bytes) {
    isize result = ::ReadFile(_handle, bytes.buf(), sizeOf(bytes), nullptr, nullptr);

    if (result == 0) {
        if (::GetLastError() == ERROR_BROKEN_PIPE) {
            return Ok(0); // EOF
        }
        return Win32::fromGetLastError();
    }

    return Ok(static_cast<usize>(result));
}

Res<usize> Fd::write(Bytes bytes) {
    isize result = ::WriteFile(_handle, bytes.buf(), sizeOf(bytes), nullptr, nullptr);

    if (result == 0) {
        if (::GetLastError() == ERROR_BROKEN_PIPE) {
            return Ok(0); // EOF
        }
        return Win32::fromGetLastError();
    }

    return Ok(static_cast<usize>(result));
}

Res<usize> Fd::seek(Io::Seek seek) {
    LARGE_INTEGER offset;
    offset.QuadPart = seek.offset;

    DWORD moveMethod;
    switch (seek.whence) {
    case Io::Whence::BEGIN:
        moveMethod = FILE_BEGIN;
        break;
    case Io::Whence::CURRENT:
        moveMethod = FILE_CURRENT;
        break;
    case Io::Whence::END:
        moveMethod = FILE_END;
        break;
    }

    if (::SetFilePointerEx(_handle, offset, &offset, moveMethod) == 0) {
        return Win32::fromGetLastError();
    }

    return Ok(static_cast<usize>(offset.QuadPart));
}

Res<> Fd::flush() {
    // NOTE: No-op
    return Ok();
}

Res<Rc<Sys::Fd>> Fd::dup() {
    return Error::notImplemented("Win32 Fd::dup not implemented");
}

Res<Sys::_Accepted> Fd::accept() {
    return Error::notImplemented("Win32 Fd::accept not implemented");
}

Res<Sys::Stat> Fd::stat() {
    return Error::notImplemented("Win32 Fd::stat not implemented");
}

Res<Sys::_Sent> Fd::send(Bytes bytes, Slice<Sys::Handle> hnds, Sys::SocketAddr addr) {
    return Error::notImplemented("Win32 Fd::send not implemented");
}

Res<Sys::_Received> Fd::recv(MutBytes bytes, MutSlice<Sys::Handle>) {
    return Error::notImplemented("Win32 Fd::recv not implemented");
}

Res<> Fd::pack(Io::PackEmit& e) {
    return Error::notImplemented("Win32 Fd::pack not implemented");
}

} // namespace Posix
