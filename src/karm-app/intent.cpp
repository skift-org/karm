export module Karm.App:intent;

import Karm.Core;
import Karm.Ref;
import Karm.App.Base;

import :_embed;

namespace Karm::App {

export Res<> launch(Intent const& intent) {
    return _Embed::launch(intent);
}

export Async::Task<> launchAsync(Intent intent) {
    co_return co_await _Embed::launchAsync(intent);
}

} // namespace Karm::App
