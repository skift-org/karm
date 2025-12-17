export module Karm.Core:async.cancellation;

import :base.rc;
import :base.res;

namespace Karm::Async {

export struct Cancellation : Meta::Pinned {
    struct Token {
        Rc<Cancellation> _c;

        Token(Rc<Cancellation> c) : _c{c} {}

        bool cancelled() const {
            return _c->cancelled();
        }

        Res<> errorIfCanceled() const {
            if (cancelled())
                return Error::interrupted("operation cancelled");
            return Ok();
        }
    };

    static Tuple<Rc<Cancellation>, Token> create() {
        auto cancellation = makeRc<Cancellation>();
        return {cancellation, Token{cancellation}};
    }

    bool _cancelled = false;

    void cancel() {
        _cancelled = true;
    }

    void reset() {
        _cancelled = false;
    }

    bool cancelled() const {
        return _cancelled;
    }
};

export using CancellationToken = Cancellation::Token;

} // namespace Karm::Async
