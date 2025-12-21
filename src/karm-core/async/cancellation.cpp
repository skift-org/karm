module;

#include <karm-core/macros.h>

export module Karm.Core:async.cancellation;

import :base.list;
import :base.res;

namespace Karm::Async {

export struct Cancellation;

export struct CancellationToken;

export struct Cancellable : Meta::Pinned {
    Cancellation* cancellation = nullptr;
    LlItem<Cancellable> item = {};

    virtual ~Cancellable();

    Res<> attach(CancellationToken ct);
    Res<> attach(Cancellation& ct);

    virtual void cancel() = 0;
};

export struct CancellationToken {
    Cancellation* cancellation;

    static CancellationToken uninterruptible() {
        return CancellationToken{nullptr};
    }

    explicit CancellationToken(Cancellation* c) : cancellation{c} {}

    bool cancelled() const;

    Res<> errorIfCanceled() const {
        if (cancelled())
            return Error::interrupted("operation cancelled");
        return Ok();
    }
};

export struct Cancellation : Cancellable {
    Ll<Cancellable> cancellables;
    bool _cancelled = false;

    ~Cancellation() {
        while (cancellables.head()) {
            auto* c = cancellables.detach(cancellables.head());
            c->cancellation = nullptr;
        }
    }

    void cancel() override {
        if (_cancelled)
            return;
        _cancelled = true;
        Ll<Cancellable> pending = std::move(cancellables);
        while (pending.head()) {
            auto* c = pending.detach(pending.head());
            c->cancellation = nullptr;
            c->cancel();
        }
    }

    void reset() {
        _cancelled = false;
    }

    bool cancelled() const {
        return _cancelled;
    }

    CancellationToken token() lifetimebound {
        return CancellationToken{this};
    }
};

bool CancellationToken::cancelled() const {
    if (not cancellation)
        return false;
    return cancellation->cancelled();
}

Res<> Cancellable::attach(CancellationToken ct) {
    if (not ct.cancellation)
        return Ok();
    return attach(*ct.cancellation);
}

Res<> Cancellable::attach(Cancellation& c) {
    if (cancellation)
        cancellation->cancellables.detach(this);

    if (c.cancelled()) {
        cancel();
        return c.token().errorIfCanceled();
    }

    cancellation = &c;
    c.cancellables.append(this, nullptr);
    return Ok();
}

Cancellable::~Cancellable() {
    if (cancellation)
        cancellation->cancellables.detach(this);
}

} // namespace Karm::Async
