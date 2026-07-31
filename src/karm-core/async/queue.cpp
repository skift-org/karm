module;

#include <karm/macros>

export module Karm.Core:async.queue;

import :async.awaiter;
import :async.cancellation;
import :async.semaphore;
import :async.task;
import :base.list;
import :base.slice;
import :base.vec;

namespace Karm::Async {

export template <typename T>
struct Queue {
    Vec<T> _buf;
    Semaphore _sem{0};

    Queue() = default;

    template <typename... Ts>
    void emplace(Ts&&... arg) {
        _buf.emplaceBack(std::forward<Ts>(arg)...);
        _sem.release();
    }

    void enqueue(T item) {
        emplace(std::move(item));
    }

    Task<T> dequeueAsync(CancellationToken ct) {
        co_trya$(_sem.acquireAsync(ct));
        co_return Ok(_buf.popFront());
    }

    bool empty() {
        return isEmpty(_buf);
    }

    Opt<T> tryDequeue() {
        if (not _sem.tryAcquire())
            return NONE;
        return _buf.popFront();
    }
};

} // namespace Karm::Async
