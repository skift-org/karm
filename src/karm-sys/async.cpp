export module Karm.Sys:async;

import Karm.Core;

import :fd;
import :_embed;

namespace Karm::Sys {

export struct Sched :
    Meta::Pinned {

    Opt<Res<>> _ret;

    virtual ~Sched() = default;

    bool exited() const { return _ret.has(); }

    void quit(Res<> ret) { _ret = ret; }

    virtual Res<> wait(Instant until) = 0;

    virtual Async::Task<usize> readAsync(Rc<Fd>, MutBytes, Async::CancellationToken ct) = 0;

    virtual Async::Task<usize> writeAsync(Rc<Fd>, Bytes, Async::CancellationToken ct) = 0;

    virtual Async::Task<> flushAsync(Rc<Fd>, Async::CancellationToken ct) = 0;

    virtual Async::Task<_Accepted> acceptAsync(Rc<Fd>, Async::CancellationToken ct) = 0;

    virtual Async::Task<_Sent> sendAsync(Rc<Fd>, Bytes, Slice<Handle>, SocketAddr, Async::CancellationToken ct) = 0;

    virtual Async::Task<_Received> recvAsync(Rc<Fd>, MutBytes, MutSlice<Handle>, Async::CancellationToken ct) = 0;

    virtual Async::Task<> sleepAsync(Instant until, Async::CancellationToken ct) = 0;
};

export Sched& globalSched() {
    return _Embed::globalSched();
}

export template <Async::Sender S>
auto run(S s, Sched& sched = globalSched()) {
    return Async::run(std::move(s), [&] {
        (void)sched.wait(Instant::endOfTime());
    });
}

} // namespace Karm::Sys
