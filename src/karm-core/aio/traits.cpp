export module Karm.Core:aio.traits;

import :async.task;

namespace Karm::Aio {

export struct Stream {
    virtual ~Stream() = default;

    virtual Async::Task<usize> writeAsync(Bytes) {
        co_return Error::unsupported("not writable");
    }

    virtual Async::Task<usize> readAsync(MutBytes) {
        co_return Error::unsupported("not readable");
    }

    virtual Async::Task<> flushAsync() {
        co_return Ok();
    }
};

} // namespace Karm::Aio
