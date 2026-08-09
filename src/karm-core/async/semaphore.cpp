module;

#include <karm/macros>

export module Karm.Core:async.semaphore;

import :async.awaiter;
import :async.cancellation;
import :async.task;
import :base.list;
import :base.slice;
import :base.vec;

namespace Karm::Async {

export struct Semaphore {

    struct _Listener : Cancellable {
        LlItem<_Listener> item;
        usize _count;
        Semaphore& _s;
        CancellationToken _ct;
        bool _cancelled = false;

        _Listener(usize count, Semaphore& s, CancellationToken ct)
            : _count(count), _s{s}, _ct{ct} {}

        virtual void complete() = 0;
    };

    usize _currentCount;
    usize _maxCount;
    Ll<_Listener> _listeners;

    Semaphore(usize initialCount = 1, usize maxCount = Limits<usize>::MAX)
        : _currentCount(initialCount), _maxCount(maxCount) {
    }

    void release(usize count = 1) {
        if (_currentCount + count > _maxCount) [[unlikely]]
            panic("semaphore is full");

        if (_listeners.empty()) {
            _currentCount += count;
            return;
        }

        if (_listeners.head()->_count <= _currentCount) [[unlikely]]
            panic("invalid listeners state");

        usize newCount = _currentCount + count;
        while (not _listeners.empty() and newCount >= _listeners.head()->_count) {
            auto listener = _listeners.detach(_listeners.head());
            newCount -= listener->_count;
            listener->complete();
        }
        _currentCount = newCount;
    }

    template <Receiver<Res<>> R>
    struct _AcquireOperation : _Listener {
        R _r;

        _AcquireOperation(usize count, Semaphore& s, CancellationToken ct, R r)
            : _Listener(count, s, ct), _r{std::move(r)} {}

        bool start() {
            if (_cancelled)
                return true;

            if (_ct.cancelled()) {
                _r.recv(INLINE, Error::interrupted("operation cancelled"));
                _cancelled = true;
                return true;
            }

            // Should only fail if ct is cancelled, which should be caught before, so we panic.
            attach(_ct).unwrap();

            if (_s._currentCount < _count or not _s._listeners.empty()) {
                _s._listeners.append(this, _s._listeners.tail());
                return false;
            }

            if (not _s._listeners.empty() and _s._listeners.head()->_count >= _s._currentCount) [[unlikely]]
                panic("invalid listeners state");

            _s._currentCount -= _count;
            _r.recv(INLINE, Ok());
            return true;
        }

        void complete() override {
            _r.recv(LATER, Ok());
        }

        void cancel() override {
            if (not _cancelled) {
                _s._listeners.detach(this);
                _r.recv(LATER, Error::interrupted("operation cancelled"));
                _cancelled = true;
            }
        }
    };

    struct _AcquireSender {
        using Inner = Res<>;
        usize _count;
        Semaphore& _s;
        CancellationToken _ct;

        auto connect(Receiver<Res<>> auto r) -> _AcquireOperation<decltype(r)> {
            return {_count, _s, _ct, std::move(r)};
        }
    };

    auto acquireAsync(usize count, CancellationToken ct) {
        return _AcquireSender{count, *this, ct};
    }

    auto acquireAsync(CancellationToken ct) {
        return acquireAsync(1, ct);
    }

    bool tryAcquire(usize count = 1) {
        if (_currentCount < count or not _listeners.empty())
            return false;

        _currentCount -= count;
        return true;
    }

    struct [[nodiscard]] Scope : Meta::NoCopy {
        Semaphore* _sema = nullptr;

        Scope(Semaphore& lock)
            : _sema(&lock) {}

        Scope(Scope&& other)
            : _sema(std::exchange(other._sema, nullptr)) {
        }

        ~Scope() {
            if (_sema)
                _sema->release(1);
        }

        Scope& operator=(Scope&& other) {
            std::swap(_sema, other._sema);
            return *this;
        }
    };

    Task<Scope> lockScopeAsync(CancellationToken ct) {
        co_trya$(acquireAsync(ct));
        co_return Ok(Scope{*this});
    }

    Opt<Scope> tryLockScope() {
        if (not tryAcquire())
            return NONE;
        return Some(Scope{*this});
    }

    ~Semaphore() {
        while (not _listeners.empty()) {
            auto op = _listeners.head();
            op->cancel();
        }
    }
};

} // namespace Karm::Async
