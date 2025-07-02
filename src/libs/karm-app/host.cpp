module;

#include <karm-base/res.h>

export module Karm.App:host;

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
