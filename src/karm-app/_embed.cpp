export module Karm.App:_embed;

import Karm.App.Base;
import Karm.Core;
import Karm.Sys;

namespace Karm::App {

export struct Prefs;
export struct Application;
export struct ApplicationProps;

} // namespace Karm::App

namespace Karm::App::_Embed {

export Prefs& globalPrefs();

export Async::Task<Rc<Application>> createAppAsync(Sys::Env& env, ApplicationProps const& props, Async::CancellationToken ct);

export Res<> launch(Intent const& intent);

export Async::Task<> launchAsync(Intent intent);

} // namespace Karm::App::_Embed
