module;

#include <errno.h>
#include <karm-core/macros.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "../posix/fd.h"
#include "../posix/utils.h"

module Karm.Sys;

import Karm.Core;

namespace Karm::Sys::_Embed {

struct EpollSched : Sys::Sched {
    int _epollFd;
    usize _id = 0;
    Map<usize, Async::Promise<>> _promises;

    EpollSched(int epollFd)
        : _epollFd(epollFd) {}

    ~EpollSched() {
        close(_epollFd);
    }

    [[clang::coro_wrapper]]
    Async::Task<> waitFor(epoll_event ev, int fd) {
        usize id = _id++;
        auto promise = Async::Promise<>();
        auto future = promise.future();

        ev.data.u64 = id;

        // Bypass things epoll won't touch (regular files, directories, many procfs entries).
        struct stat st{};
        if (fstat(fd, &st) == 0 && (S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))) {
            // Treat as immediately "ready" for our purposes.
            return Async::makeTask(Async::One<Res<>>{Ok()});
        }

        // Try to add to epoll
        if (::epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            if (errno == EEXIST) {
                // Already tracked: just update its interest set.
                if (::epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) == 0) {
                    _promises.put(id, std::move(promise));
                    return Async::makeTask(future);
                }
                return Async::makeTask(Async::One<Res<>>{Posix::fromLastErrno()});
            }

            if (errno == EPERM) {
                // Not epollable (e.g., regular file, some character devices). Consider it ready.
                return Async::makeTask(Async::One<Res<>>{Ok()});
            }

            return Async::makeTask(Async::One<Res<>>{Posix::fromLastErrno()});
        }

        _promises.put(id, std::move(promise));
        return Async::makeTask(future);
    }

    Async::Task<usize> readAsync(Rc<Fd> fd, MutBytes buf) override {
        int rawFd = co_try$(Posix::toPosixFd(fd))->_raw;
        co_trya$(waitFor({.events = EPOLLIN | EPOLLET, .data = {}}, rawFd));
        co_return Ok(co_try$(fd->read(buf)));
    }

    Async::Task<usize> writeAsync(Rc<Fd> fd, Bytes buf) override {
        int rawFd = co_try$(Posix::toPosixFd(fd))->_raw;
        co_trya$(waitFor({.events = EPOLLOUT | EPOLLET, .data = {}}, rawFd));
        co_return Ok(co_try$(fd->write(buf)));
    }

    Async::Task<> flushAsync(Rc<Fd> fd) override {
        int rawFd = co_try$(Posix::toPosixFd(fd))->_raw;
        co_trya$(waitFor({.events = EPOLLOUT | EPOLLET, .data = {}}, rawFd));
        co_return Ok(co_try$(fd->flush()));
    }

    Async::Task<_Accepted> acceptAsync(Rc<Fd> fd) override {
        int rawFd = co_try$(Posix::toPosixFd(fd))->_raw;
        co_trya$(waitFor({.events = EPOLLIN | EPOLLET, .data = {}}, rawFd));
        co_return Ok(co_try$(fd->accept()));
    }

    Async::Task<_Sent> sendAsync(Rc<Fd> fd, Bytes buf, Slice<Handle> handles, SocketAddr addr) override {
        int rawFd = co_try$(Posix::toPosixFd(fd))->_raw;
        co_trya$(waitFor({.events = EPOLLOUT | EPOLLET, .data = {}}, rawFd));
        co_return Ok(co_try$(fd->send(buf, handles, addr)));
    }

    Async::Task<_Received> recvAsync(Rc<Fd> fd, MutBytes buf, MutSlice<Handle> hnds) override {
        int rawFd = co_try$(Posix::toPosixFd(fd))->_raw;
        co_trya$(waitFor({.events = EPOLLIN | EPOLLET, .data = {}}, rawFd));
        co_return Ok(co_try$(fd->recv(buf, hnds)));
    }

    Async::Task<> sleepAsync(Instant until) override {
        Instant instant = Sys::instant();
        Duration delta = Duration::zero();
        if (instant < until)
            delta = until - instant;

        int timeFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (timeFd < 0)
            co_return Posix::fromLastErrno();
        Defer _{[&] {
            close(timeFd);
        }};

        itimerspec spec{};
        spec.it_value = Posix::toTimespec(delta);
        if (timerfd_settime(timeFd, 0, &spec, nullptr) < 0)
            co_return Posix::fromLastErrno();

        co_trya$(waitFor({.events = EPOLLIN, .data = {}}, timeFd));

        co_return Ok();
    }

    Res<> wait(Instant until) override {
        epoll_event ev;
        auto instant = Sys::instant();
        Duration delta = Duration::zero();
        if (instant < until)
            delta = until - instant;
        int timeout = until.isEndOfTime() ? -1 : delta.toMSecs();

        int n = ::epoll_wait(_epollFd, &ev, 1, timeout);

        if (n < 0)
            return Posix::fromLastErrno();

        if (n == 0)
            return Ok();

        usize id = ev.data.u64;
        auto promise = _promises.take(id);
        promise.resolve(Ok());
        return Ok();
    }
};

Sched& globalSched() {
    static EpollSched sched = [] {
        int fd = ::epoll_create1(0);
        if (fd < 0)
            panic("epoll_create1");
        return EpollSched(fd);
    }();
    return sched;
}

} // namespace Karm::Sys::_Embed
