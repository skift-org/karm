module;

#include <liburing.h>
#include <poll.h>
//
#include <karm-core/macros.h>
#include <karm-sys/posix/fd.h>
#include <karm-sys/posix/utils.h>

module Karm.Sys;

import Karm.Core;

namespace Karm::Sys::_Embed {

struct __kernel_timespec toKernelTimespec(Instant ts) {
    struct __kernel_timespec kts;
    if (ts.isEndOfTime()) {
        kts.tv_sec = LONG_MAX;
        kts.tv_nsec = 0;
        return kts;
    } else {
        kts.tv_sec = ts.val() / 1000000;
        kts.tv_nsec = (ts.val() % 1000000) * 1000;
    }
    return kts;
}

struct __kernel_timespec toKernelTimespec(Duration ts) {
    struct __kernel_timespec kts;
    if (ts.isInfinite()) {
        kts.tv_sec = LONG_MAX;
        kts.tv_nsec = 0;
        return kts;
    } else {
        kts.tv_sec = ts.val() / 1000000;
        kts.tv_nsec = (ts.val() % 1000000) * 1000;
    }
    return kts;
}

static char CANCEL_TOKEN = 0xff;

struct UringSched : Sys::Sched {
    static constexpr auto NCQES = 128;

    struct _Job : Async::Cancellable {
        UringSched& _uring;

        _Job(UringSched& uring) : _uring(uring) {}

        virtual void submit(io_uring_sqe* sqe) = 0;
        virtual void complete(io_uring_cqe* cqe) = 0;

        void cancel() override {
            io_uring_sqe* sqe = io_uring_get_sqe(&_uring._ring);
            if (not sqe) [[unlikely]]
                panic("failed to get sqe");
            io_uring_prep_cancel64(sqe, reinterpret_cast<usize>(this), 0);
            sqe->user_data = reinterpret_cast<usize>(&CANCEL_TOKEN);
            io_uring_submit(&_uring._ring);
        }
    };

    io_uring _ring;

    UringSched(io_uring ring)
        : _ring(ring) {}

    ~UringSched() {
        io_uring_queue_exit(&_ring);
    }

    void submit(_Job& job, Async::CancellationToken ct) {
        auto* sqe = io_uring_get_sqe(&_ring);
        if (not sqe) [[unlikely]]
            panic("failed to get sqe");
        job.submit(sqe);
        sqe->user_data = reinterpret_cast<usize>(&job);
        (void)job.attach(ct);
        io_uring_submit(&_ring);
    }

    [[clang::coro_wrapper]]
    Async::Task<usize> readAsync(Rc<Fd> fd, MutBytes buf, Async::CancellationToken ct) override {
        struct Job : _Job {
            Rc<Posix::Fd> _fd;
            MutBytes _buf;
            Async::Promise<usize> _promise;

            Job(UringSched& uring, Rc<Posix::Fd> fd, MutBytes buf)
                : _Job(uring), _fd(fd), _buf(buf) {}

            void submit(io_uring_sqe* sqe) override {
                io_uring_prep_read(
                    sqe,
                    _fd->_raw,
                    _buf.buf(),
                    _buf.len(),
                    -1
                );
            }

            void complete(io_uring_cqe* cqe) override {
                auto res = cqe->res;
                if (res < 0)
                    _promise.resolve(Posix::fromErrno(-cqe->res));
                else
                    _promise.resolve(Ok(cqe->res));
            }

            auto future() {
                return _promise.future();
            }
        };

        co_try$(ct.errorIfCanceled());
        Job job{*this, co_try$(Posix::toPosixFd(fd)), buf};
        submit(job, ct);
        co_return co_await job.future();
    }

    [[clang::coro_wrapper]]
    Async::Task<usize> writeAsync(Rc<Fd> fd, Bytes buf, Async::CancellationToken ct) override {
        struct Job : _Job {
            Rc<Posix::Fd> _fd;
            Bytes _buf;
            Async::Promise<usize> _promise;

            Job(UringSched& uring, Rc<Posix::Fd> fd, Bytes buf)
                : _Job(uring), _fd(fd), _buf(buf) {}

            void submit(io_uring_sqe* sqe) override {
                io_uring_prep_write(
                    sqe,
                    _fd->_raw,
                    _buf.buf(),
                    _buf.len(),
                    // NOTE: On files that support seeking, if the offset is set
                    //       to -1, the write operation commences at the file
                    //       offset, and the file offset is incremented by
                    //       the number of bytes written. See io_uring_prep_write(3).
                    -1
                );
            }

            void complete(io_uring_cqe* cqe) override {
                auto res = cqe->res;
                if (res < 0)
                    _promise.resolve(Posix::fromErrno(-cqe->res));
                else
                    _promise.resolve(Ok(cqe->res));
            }

            auto future() {
                return _promise.future();
            }
        };

        co_try$(ct.errorIfCanceled());
        Job job{*this, co_try$(Posix::toPosixFd(fd)), buf};
        submit(job, ct);
        co_return co_await job.future();
    }

    [[clang::coro_wrapper]]
    Async::Task<> flushAsync(Rc<Fd> fd, Async::CancellationToken ct) override {
        struct Job : _Job {
            Rc<Posix::Fd> _fd;
            Async::Promise<> _promise;

            Job(UringSched& uring, Rc<Posix::Fd> fd)
                : _Job(uring), _fd(fd) {}

            void submit(io_uring_sqe* sqe) override {
                io_uring_prep_fsync(sqe, _fd->_raw, 0);
            }

            void complete(io_uring_cqe* cqe) override {
                auto res = cqe->res;
                if (res < 0)
                    _promise.resolve(Posix::fromErrno(-cqe->res));
                else
                    _promise.resolve(Ok());
            }

            auto future() {
                return _promise.future();
            }
        };

        co_try$(ct.errorIfCanceled());
        Job job{*this, co_try$(Posix::toPosixFd(fd))};
        submit(job, ct);
        co_return co_await job.future();
    }

    [[clang::coro_wrapper]]
    Async::Task<_Accepted> acceptAsync(Rc<Fd> fd, Async::CancellationToken ct) override {
        struct Job : _Job {
            Rc<Posix::Fd> _fd;
            sockaddr_in _addr{};
            unsigned _addrLen = sizeof(sockaddr_in);
            Async::Promise<_Accepted> _promise;

            Job(UringSched& uring, Rc<Posix::Fd> fd)
                : _Job(uring), _fd(fd) {}

            void submit(io_uring_sqe* sqe) override {
                io_uring_prep_accept(sqe, _fd->_raw, (struct sockaddr*)&_addr, &_addrLen, 0);
            }

            void complete(io_uring_cqe* cqe) override {
                auto res = cqe->res;
                if (res < 0)
                    _promise.resolve(Posix::fromErrno(-cqe->res));
                else {
                    _Accepted accepted = {makeRc<Posix::Fd>(res), Posix::fromSockAddr(_addr)};
                    _promise.resolve(Ok(accepted));
                }
            }

            auto future() {
                return _promise.future();
            }
        };

        co_try$(ct.errorIfCanceled());
        Job job{*this, co_try$(Posix::toPosixFd(fd))};
        submit(job, ct);
        co_return co_await job.future();
    }

    [[clang::coro_wrapper]]
    Async::Task<_Sent> sendAsync(Rc<Fd> fd, Bytes buf, Slice<Handle> handles, SocketAddr addr, Async::CancellationToken ct) override {
        if (handles.len() > 0)
            notImplemented(); // TODO: Implement handle passing on POSIX

        struct Job : _Job {
            Rc<Posix::Fd> _fd;
            Bytes _buf;
            iovec _iov;
            msghdr _msg;
            sockaddr_in _addr;
            Async::Promise<_Sent> _promise;

            Job(UringSched& uring, Rc<Posix::Fd> fd, Bytes buf, SocketAddr addr)
                : _Job(uring), _fd(fd), _buf(buf), _addr(Posix::toSockAddr(addr)) {}

            void submit(io_uring_sqe* sqe) override {
                _iov.iov_base = const_cast<u8*>(_buf.begin());
                _iov.iov_len = _buf.len();

                _msg.msg_name = &_addr;
                _msg.msg_namelen = sizeof(sockaddr_in);
                _msg.msg_iov = &_iov;
                _msg.msg_iovlen = 1;

                io_uring_prep_sendmsg(
                    sqe,
                    _fd->_raw,
                    &_msg,
                    0
                );
            }

            void complete(io_uring_cqe* cqe) override {
                if (cqe->res < 0)
                    _promise.resolve(Posix::fromErrno(-cqe->res));
                else
                    _promise.resolve(Ok<_Sent>(cqe->res, 0));
            }

            auto future() {
                return _promise.future();
            }
        };

        co_try$(ct.errorIfCanceled());
        Job job{*this, co_try$(Posix::toPosixFd(fd)), buf, addr};
        submit(job, ct);
        co_return co_await job.future();
    }

    [[clang::coro_wrapper]]
    Async::Task<_Received> recvAsync(Rc<Fd> fd, MutBytes buf, MutSlice<Handle>, Async::CancellationToken ct) override {
        struct Job : _Job {
            Rc<Posix::Fd> _fd;
            MutBytes _buf;
            iovec _iov;
            msghdr _msg;
            sockaddr_in _addr;
            Async::Promise<_Received> _promise;

            Job(UringSched& uring, Rc<Posix::Fd> fd, MutBytes buf)
                : _Job(uring), _fd(fd), _buf(buf) {}

            void submit(io_uring_sqe* sqe) override {
                _iov.iov_base = _buf.begin();
                _iov.iov_len = _buf.len();

                _msg.msg_name = &_addr;
                _msg.msg_namelen = sizeof(sockaddr_in);
                _msg.msg_iov = &_iov;
                _msg.msg_iovlen = 1;

                io_uring_prep_recvmsg(sqe, _fd->_raw, &_msg, 0);
            }

            void complete(io_uring_cqe* cqe) override {
                if (cqe->res < 0)
                    _promise.resolve(Posix::fromErrno(-cqe->res));
                else {
                    _Received received = {(usize)cqe->res, 0, Posix::fromSockAddr(_addr)};
                    _promise.resolve(Ok(received));
                }
            }

            auto future() {
                return _promise.future();
            }
        };

        co_try$(ct.errorIfCanceled());
        Job job{*this, co_try$(Posix::toPosixFd(fd)), buf};
        submit(job, ct);
        co_return co_await job.future();
    }

    Async::Task<Flags<Poll>> pollAsync(Rc<Fd> fd, Flags<Sys::Poll> events, Async::CancellationToken ct) override {
        struct Job : _Job {
            Rc<Posix::Fd> _fd;
            Flags<Sys::Poll> _events;
            Async::Promise<Flags<Sys::Poll>> _promise;

            Job(UringSched& uring, Rc<Posix::Fd> fd, Flags<Sys::Poll> events)
                : _Job(uring), _fd(fd), _events(events) {}

            void submit(io_uring_sqe* sqe) override {
                unsigned pollMask = 0;
                if (_events.has(Poll::READABLE))
                    pollMask |= POLLIN;
                if (_events.has(Poll::WRITEABLE))
                    pollMask |= POLLOUT;
                io_uring_prep_poll_add(sqe, _fd->_raw, pollMask);
            }

            void complete(io_uring_cqe* cqe) override {
                auto res = cqe->res;
                if (res < 0)
                    _promise.resolve(Posix::fromErrno(-cqe->res));
                else {
                    Flags<Sys::Poll> events = {};
                    if (cqe->res & POLLIN)
                        events.set(Poll::READABLE);
                    if (cqe->res & POLLOUT)
                        events.set(Poll::WRITEABLE);
                    _promise.resolve(Ok(events));
                }
            }

            auto future() {
                return _promise.future();
            }
        };

        co_try$(ct.errorIfCanceled());
        Job job{*this, co_try$(Posix::toPosixFd(fd)), events};
        submit(job, ct);
        co_return co_await job.future();
    }

    [[clang::coro_wrapper]]
    Async::Task<> sleepAsync(Instant until, Async::CancellationToken ct) override {
        struct Job : _Job {
            Instant _until;
            Async::Promise<> _promise;

            struct __kernel_timespec _ts{};

            Job(UringSched& uring, Instant until)
                : _Job(uring), _until(until) {}

            void submit(io_uring_sqe* sqe) override {
                _ts = toKernelTimespec(_until);
                io_uring_prep_timeout(sqe, &_ts, 0, IORING_TIMEOUT_ABS);
            }

            void complete(io_uring_cqe* cqe) override {
                if (cqe->res < 0 and cqe->res != -ETIME)
                    _promise.resolve(Posix::fromErrno(-cqe->res));
                else
                    _promise.resolve(Ok());
            }

            auto future() {
                return _promise.future();
            }
        };

        co_try$(ct.errorIfCanceled());
        Job job{*this, until};
        submit(job, ct);
        co_return co_await job.future();
    }

    bool _inWait = false;

    Res<> wait(Instant until) override {
        if (_inWait)
            panic("nested wait");
        _inWait = true;
        Defer _{[&] {
            _inWait = false;
        }};

        Instant now = Sys::instant();

        Duration delta = Duration::zero();
        if (now < until)
            delta = until - now;

        struct __kernel_timespec ts = toKernelTimespec(delta);
        io_uring_cqe* cqe = nullptr;
        io_uring_wait_cqe_timeout(&_ring, &cqe, &ts);

        unsigned head;
        usize i = 0;
        io_uring_for_each_cqe(&_ring, head, cqe) {
            ++i;
            if (cqe->user_data == reinterpret_cast<usize>(&CANCEL_TOKEN)) {
                continue;
            }

            auto* job = reinterpret_cast<_Job*>(cqe->user_data);
            job->complete(cqe);
        }

        io_uring_cq_advance(&_ring, i);

        return Ok();
    }
};

Sched& globalSched() {
    static UringSched sched = [] {
        io_uring ring{};
        auto res = io_uring_queue_init(UringSched::NCQES, &ring, 0);
        if (res < 0) [[unlikely]]
            panic("failed to initialize io_uring");
        return UringSched(ring);
    }();
    return sched;
}

} // namespace Karm::Sys::_Embed
