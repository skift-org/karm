export module Karm.App.Base:host;

import Karm.Core;
import Karm.Math;
import :event;
import :formFactor;

namespace Karm::App {

export struct RequestCloseEvent {
    Res<> res = Ok();
};

export struct RequestMinimizeEvent {
};

export enum struct Snap {
    NONE,
    FULL,
    LEFT,
    RIGHT,

    _LEN,
};

export struct RequestSnapeEvent {
    Snap snap;
};

export struct ResizeEvent {
    Math::Vec2i size;
};

export struct FormfactorEvent {
    FormFactor formFactor;
};

export struct DragStartEvent {
};

} // namespace Karm::App
