export module Karm.Sys:pty;

import :_embed;
import :async;
import :fd;

namespace Karm::Sys {

export struct Pty :
    Aio::Reader,
    Io::Reader,
    Aio::Writer,
    Io::Writer {
    Rc<Fd> _fd;

    Pty(Rc<Fd> fd) : _fd(fd) {}

    Res<usize> read(MutBytes bytes) override {
        return _fd->read(bytes);
    }

    [[clang::coro_wrapper]]
    Async::Task<usize> readAsync(MutBytes bytes, Async::CancellationToken ct) override {
        return globalSched().readAsync(_fd, bytes, ct);
    }

    Res<usize> write(Bytes bytes) override {
        return _fd->write(bytes);
    }

    [[clang::coro_wrapper]]
    Async::Task<usize> writeAsync(Bytes bytes, Async::CancellationToken ct) override {
        return globalSched().writeAsync(_fd, bytes, ct);
    }
};

} // namespace Karm::Sys