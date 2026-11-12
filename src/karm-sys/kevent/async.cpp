module;

#include <karm/macros>
#include <sys/event.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __ck_sys_freebsd__
#    define kevent64 kevent
#    define kevent64_s kevent
#endif

module Karm.Sys;

import Karm.Sys.Posix;
import Karm.Core;

namespace Karm::Sys::_Embed {

struct DarwinSched :
    Sys::Sched {

    struct _Job : Async::Cancellable {
        DarwinSched& _sched;
        usize _id;
        Opt<struct kevent64_s> _ev;
        Async::Promise<> _promise;

        _Job(DarwinSched& sched, usize id)
            : _sched(sched), _id(id) {}

        void cancel() override {
            if (not _sched._jobs.remove(_id))
                return;

            if (_ev) {
                auto ev = *_ev;
                ev.flags = EV_DELETE;
                (void)_sched._submit(ev);
            }

            _promise.resolve(Error::interrupted("operation cancelled"));
        }
    };

    int _kqueue;
    usize _id = 0;
    Map<usize, _Job*> _jobs;

    DarwinSched(int kqueue)
        : _kqueue(kqueue) {
    }

    ~DarwinSched() {
        close(_kqueue);
    }

    struct timespec _computeTimeout(Instant until) {
        Instant now = Sys::instant();
        Duration delta = Duration::ZERO;
        if (now < until)
            delta = until - now;
        return Posix::toTimespec(delta);
    }

    Res<> _submit(struct kevent64_s ev) {
        int res = ::kevent64(
            _kqueue,

            &ev,
            1,

            nullptr,
            0,

#ifdef __ck_sys_darwin__
            0,
#endif
            nullptr
        );

        if (res < 0)
            return Posix::fromLastErrno();
        return Ok();
    }

    Async::Task<> _waitAsync(Opt<struct kevent64_s> ev, Async::CancellationToken ct) {
        if (ev and (ev->filter == EVFILT_READ or ev->filter == EVFILT_WRITE)) {
            // kqueue never reports a regular file as readable once its offset is at EOF.
            struct stat st{};
            if (fstat((int)ev->ident, &st) == 0 and (S_ISREG(st.st_mode) or S_ISDIR(st.st_mode)))
                co_return Ok();
        }

        co_try$(ct.errorIfCanceled());

        _Job job{*this, _id++};
        if (ev) {
            if (ev->filter == EVFILT_TIMER)
                ev->ident = job._id;
            ev->udata = (decltype(ev->udata))job._id;
            co_try$(_submit(*ev));
            job._ev = ev;
        }

        _jobs.put(job._id, &job);
        co_try$(job.attach(ct));
        co_return co_await job._promise.future();
    }

    [[clang::coro_wrapper]]
    Async::Task<> waitForAsync(struct kevent64_s ev, Async::CancellationToken ct) {
        return _waitAsync(Some(ev), ct);
    }

    Async::Task<usize> readAsync(Rc<Fd> fd, MutBytes buf, Async::CancellationToken ct) override {
        co_trya$(waitForAsync({
            .ident = (u64)co_try$(Posix::ensurePosixFd(fd))->_raw,
            .filter = EVFILT_READ,
            .flags = EV_ADD | EV_ONESHOT,
            .fflags = 0,
            .data = 0,
            .udata = 0,
            .ext = {},
        }, ct));
        co_return Ok(co_try$(fd->read(buf)));
    }

    Async::Task<usize> writeAsync(Rc<Fd> fd, Bytes buf, Async::CancellationToken ct) override {
        co_trya$(waitForAsync({
            .ident = (u64)co_try$(Posix::ensurePosixFd(fd))->_raw,
            .filter = EVFILT_WRITE,
            .flags = EV_ADD | EV_ONESHOT,
            .fflags = 0,
            .data = 0,
            .udata = 0,
            .ext = {},
        }, ct));
        co_return Ok(co_try$(fd->write(buf)));
    }

    Async::Task<> flushAsync(Rc<Fd> fd, Async::CancellationToken ct) override {
        co_trya$(waitForAsync({
            .ident = (u64)co_try$(Posix::ensurePosixFd(fd))->_raw,
            .filter = EVFILT_WRITE,
            .flags = EV_ADD | EV_ONESHOT,
            .fflags = 0,
            .data = 0,
            .udata = 0,
            .ext = {},
        }, ct));
        co_return Ok(co_try$(fd->flush()));
    }

    Async::Task<_Accepted> acceptAsync(Rc<Fd> fd, Async::CancellationToken ct) override {
        co_trya$(waitForAsync({
            .ident = (u64)co_try$(Posix::ensurePosixFd(fd))->_raw,
            .filter = EVFILT_READ,
            .flags = EV_ADD | EV_ONESHOT,
            .fflags = 0,
            .data = 0,
            .udata = 0,
            .ext = {},
        }, ct));
        co_return Ok(co_try$(fd->accept()));
    }

    Async::Task<_Sent> sendAsync(Rc<Fd> fd, Bytes buf, Slice<Handle> handles, SocketAddr addr, Async::CancellationToken ct) override {
        co_trya$(waitForAsync({
            .ident = (u64)co_try$(Posix::ensurePosixFd(fd))->_raw,
            .filter = EVFILT_WRITE,
            .flags = EV_ADD | EV_ONESHOT,
            .fflags = 0,
            .data = 0,
            .udata = 0,
            .ext = {},
        }, ct));
        co_return Ok(co_try$(fd->send(buf, handles, addr)));
    }

    Async::Task<_Received> recvAsync(Rc<Fd> fd, MutBytes buf, MutSlice<Handle> hnds, Async::CancellationToken ct) override {
        co_trya$(waitForAsync({
            .ident = (u64)co_try$(Posix::ensurePosixFd(fd))->_raw,
            .filter = EVFILT_READ,
            .flags = EV_ADD | EV_ONESHOT,
            .fflags = 0,
            .data = 0,
            .udata = 0,
            .ext = {},
        }, ct));
        co_return Ok(co_try$(fd->recv(buf, hnds)));
    }

    [[clang::coro_wrapper]]
    Async::Task<> sleepAsync(Instant until, Async::CancellationToken ct) override {
        if (until.isEndOfTime())
            return _waitAsync(NONE, ct);

        Instant now = Sys::instant();
        Duration delta = Duration::ZERO;
        if (now < until)
            delta = until - now;

        return waitForAsync(
            {
                .ident = 0,
                .filter = EVFILT_TIMER,
                .flags = EV_ADD | EV_ONESHOT,
                .fflags = NOTE_USECONDS,
                .data = (i64)delta.toUSecs(),
                .udata = 0,
                .ext = {},
            },
            ct
        );
    }

    [[clang::coro_wrapper]]
    Async::Task<Flags<Poll>> pollAsync(Rc<Fd>, Flags<Poll>, Async::CancellationToken) override {
        notImplemented();
    }

    Res<> wait(Instant until) override {
        struct kevent64_s ev;
        struct timespec ts = _computeTimeout(until);

        int n = ::kevent64(
            _kqueue,

            nullptr,
            0,

            &ev,
            1,

#ifdef __ck_sys_darwin__
            0,
#endif
            until.isEndOfTime() ? nullptr : &ts
        );

        if (n < 0)
            return Posix::fromLastErrno();

        if (n == 0)
            return Ok();

        auto job = _jobs.remove((usize)ev.udata);
        if (not job)
            return Ok();

        if (ev.flags & EV_ERROR)
            (*job)->_promise.resolve(Posix::fromErrno(ev.data));
        else
            (*job)->_promise.resolve(Ok());
        return Ok();
    }
};

Sched& globalSched() {
    static DarwinSched sched = [] {
        int kqueue = ::kqueue();
        if (kqueue < 0)
            panic("kqueue");
        return DarwinSched(kqueue);
    }();
    return sched;
}

} // namespace Karm::Sys::_Embed
