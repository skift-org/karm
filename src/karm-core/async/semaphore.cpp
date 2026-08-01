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
        virtual void complete() = 0;
    };

    usize _currentCount;
    usize _maxCount;
    Ll<_Listener> _listeners;

    Semaphore(usize initialCount = 1, usize maxCount = Limits<usize>::MAX)
        : _currentCount(initialCount), _maxCount(maxCount) {
    }

    void release(usize count = 1) {
        if (_currentCount + count > _maxCount)
            panic("semaphore is full");

        if (_listeners.empty()) {
            _currentCount += count;
            return;
        }

        if (_currentCount != 0)
            panic("semaphore should be empty");

        usize newCount = _currentCount + count;
        while (newCount > 0 and not _listeners.empty()) {
            auto listener = _listeners.detach(_listeners.head());
            newCount--;
            listener->complete();
        }
        _currentCount = newCount;
    }

    template <Receiver<Res<>> R>
    struct _AcquireOperation : _Listener {
        Semaphore& _s;
        R _r;
        CancellationToken _ct;
        bool _cancelled = false;

        _AcquireOperation(Semaphore& s, R r, CancellationToken ct)
            : _s{s}, _r{std::move(r)}, _ct{ct} {}

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

            if (_s._currentCount == 0) {
                _s._listeners.append(this, _s._listeners.tail());
                return false;
            }

            if (not _s._listeners.empty())
                panic("listeners should be empty");

            _s._currentCount--;
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
        Semaphore& _s;
        CancellationToken _ct;

        auto connect(Receiver<Res<>> auto r) -> _AcquireOperation<decltype(r)> {
            return {_s, std::move(r), _ct};
        }
    };

    auto acquireAsync(CancellationToken ct) {
        return _AcquireSender{*this, ct};
    }

    bool tryAcquire() {
        if (_currentCount == 0)
            return false;
        if (not _listeners.empty())
            panic("listeners should be empty");
        _currentCount--;
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
        return Scope{*this};
    }

    ~Semaphore() {
        while (not _listeners.empty()) {
            auto op = _listeners.head();
            op->cancel();
        }
    }
};

} // namespace Karm::Async
