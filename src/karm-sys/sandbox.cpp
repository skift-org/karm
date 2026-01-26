module;

#include <karm/macros>

export module Karm.Sys:sandbox;

import Karm.Core;
import :_embed;

namespace Karm::Sys {

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
