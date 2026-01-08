export module Karm.App.Base:host;

import Karm.Core;
import Karm.Math;
import :event;

namespace Karm::App {

export struct RequestCloseEvent {
    Res<> res = Ok();
};

export struct RequestMinimizeEvent {
};

export struct RequestMaximizeEvent {
};

export struct ResizeEvent {
    Math::Vec2i size;
};

export struct DragEvent {
    enum {
        START,
        DRAG,
        END
    } type;

    Math::Vec2i delta{};
};

} // namespace Karm::App
