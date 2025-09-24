import Karm.Sys;

namespace Karm::Sys::_Embed {

Res<> hardenSandbox() {
    Sys::errln("Sandbox hardening is not supported in this environment.");
    return Ok();
}

} // namespace Karm::Sys::_Embed
