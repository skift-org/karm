module Karm.Sys;

import Karm.Core;

namespace Karm::Sys::_Embed {

Res<> hardenSandbox() {
    Sys::errln("Sandbox hardening is not supported in this environment.");
    return Ok();
}

} // namespace Karm::Sys::_Embed
