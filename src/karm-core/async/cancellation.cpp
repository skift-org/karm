export module Karm.Core:async.cancellation;

import :base.rc;
import :base.res;
import :base.list;

namespace Karm::Async {

export struct Cancellation;

export struct CancellationToken {
    Rc<Cancellation> _c;

    CancellationToken(Rc<Cancellation> c) : _c{c} {}

    bool cancelled() const {
        return _c->cancelled();
    }

    Res<> errorIfCanceled() const {
        if (cancelled())
            return Error::interrupted("operation cancelled");
        return Ok();
    }
};

export struct Cancellable {
    Rc<Cancellation> _c;
    LlItem<Cancellable> item;

    Cancellable(Cancellation::Token _ct) : _c(_ct._c){
        _c->_attach(this);
    }

    virtual void cancelled();

    virtual ~Cancellable() {
        _c->_detach(this);
    };
};

struct Cancellation : Meta::Pinned {


    static Tuple<Rc<Cancellation>, Token> create() {
        auto cancellation = makeRc<Cancellation>();
        return {cancellation, Token{cancellation}};
    }

    bool _cancelled = false;
    Ll<Cancellable> _cancellables;

    void cancel() {
        _cancelled = true;
    }

    void reset() {
        _cancelled = false;
    }

    bool cancelled() const {
        return _cancelled;
    }

    void _attach(Cancellable* cancellable) {
        _cancellables.append(cancellable, nullptr);
    }

    void _detach(Cancellable* cancellable) {
        _cancellables.detach(cancellable, nullptr);
    }
};

} // namespace Karm::Async
