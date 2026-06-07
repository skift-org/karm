module;

#include <karm/macros>

export module Karm.Core:base.defer;

import :meta.nocopy;
import :base.base;

namespace Karm {

/// Defer a computation until the end of the current scope.
/// The computation is executed in the reverse order of the defer calls.
/// Can be disarm which will prevent the computation from being executed.
export template <typename F>
struct Defer : Meta::Pinned {
    F _f;
    bool _armed = true;

    always_inline Defer(F f) : _f(f) {}

    always_inline ~Defer() {
        if (_armed)
            _f();
    }

    always_inline void disarm() {
        _armed = false;
    }

    always_inline void arm() {
        _armed = true;
    }

    always_inline void fire() {
        if (_armed)
            _f();
        disarm();
    }
};

export template <typename F>
Defer(F) -> Defer<F>;

} // namespace Karm
