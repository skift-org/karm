export module Karm.App:_embed;

import Karm.Core;
import Karm.Sys;

namespace Karm::App {

export struct Prefs;
export struct Application;
export struct ApplicationProps;

} // namespace Karm::App

namespace Karm::App::_Embed {

export Prefs& globalPrefs();

export Async::Task<Rc<Application>> createAppAsync(Sys::Context& ctx, ApplicationProps const& props, Async::CancellationToken ct);

} // namespace Karm::App::_Embed
