export module Karm.App:_embed;

import Karm.Core;

namespace Karm::App {

export struct Prefs;
export struct Application;
export struct ApplicationProps;

} // namespace Karm::App

namespace Karm::App::_Embed {

export Prefs& globalPrefs();
export Res<Rc<Application>> createApp(ApplicationProps const& props);

} // namespace Karm::App::_Embed
