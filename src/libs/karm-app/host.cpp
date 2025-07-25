export module Karm.App:host;

import :base.res;
import :event;

namespace Karm::App {

export struct RequestExitEvent {
    Res<> res = Ok();
};

export struct RequestMinimizeEvent {
};

export struct RequestMaximizeEvent {
};

} // namespace Karm::App
