module;

#include <karm-core/macros.h>

export module Karm.Sys:proc;

import :_embed;
import :time;

namespace Karm::Sys {

export Res<> sleep(Duration span) {
    return _Embed::sleep(span);
}

export Res<> sleepUntil(Instant until) {
    return _Embed::sleepUntil(until);
}

export Res<Ref::Url> pwd() {
    return _Embed::pwd();
}

export [[noreturn]] void exit(Res<> res) {
    _Embed::exit(res ? 0 : -toUnderlyingType(res.none().code()))
        .unwrap();
    unreachable();
}

// MARK: Sandboxing ------------------------------------------------------------

static bool _sandboxed = false;

export Res<> enterSandbox() {
    try$(_Embed::hardenSandbox());
    _sandboxed = true;
    return Ok();
}

export Res<> ensureUnrestricted() {
    if (_sandboxed)
        return Error::permissionDenied("sandboxed");
    return Ok();
}

} // namespace Karm::Sys
