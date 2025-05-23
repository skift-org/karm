#pragma once

#include <karm-base/rc.h>
#include <karm-base/res.h>

namespace Karm::Async {

struct Cancelation : Meta::Pinned {
    struct Token {
        Rc<Cancelation> _c;

        Token(Rc<Cancelation> c) : _c{c} {}

        bool canceled() const {
            return _c->canceled();
        }

        Res<> errorIfCanceled() const {
            if (canceled())
                return Error::interrupted("operation canceled");
            return Ok();
        }
    };

    static Tuple<Rc<Cancelation>, Token> create() {
        auto cancelation = makeRc<Cancelation>();
        return {cancelation, Token{cancelation}};
    }

    bool _canceled = false;

    void cancel() {
        _canceled = true;
    }

    void reset() {
        _canceled = false;
    }

    bool canceled() const {
        return _canceled;
    }
};

using Ct = Cancelation::Token;

} // namespace Karm::Async
