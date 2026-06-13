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

export enum struct Direction : u8 {
    EAST,
    NORTH,
    NORTH_EAST,
    NORTH_WEST,
    SOUTH,
    SOUTH_EAST,
    SOUTH_WEST,
    WEST,

    _LEN,
};

export Opt<Direction> resizeDirectionFromPos(Math::Vec2i pos, Math::Recti rect, isize grip) {
    isize x = 0, y = 0;

    if (pos.x < rect.start() + grip)
        x = -1;
    else if (pos.x >= rect.end() - grip)
        x = 1;

    if (pos.y < rect.top() + grip)
        y = -1;
    else if (pos.y >= rect.bottom() - grip)
        y = 1;

    if (x == -1)
        return y == -1  ? Direction::NORTH_WEST
               : y == 1 ? Direction::SOUTH_WEST
                        : Direction::WEST;
    if (x == 1)
        return y == -1
                   ? Direction::NORTH_EAST
               : y == 1 ? Direction::SOUTH_EAST
                        : Direction::EAST;
    if (y == -1)
        return Direction::NORTH;
    if (y == 1)
        return Direction::SOUTH;

    return NONE;
}

export struct ResizeStartEvent {
    Direction dir;
};

export struct ResizeEvent {
    Math::Vec2i size;
};

export enum struct CursorStyle {
    DEFAULT,
    RESIZE_EW,
    RESIZE_NS,
    RESIZE_NWSE,
    RESIZE_NESW,

    _LEN,
};

export CursorStyle cursorFromDirection(Direction dir) {
    switch (dir) {
    case Direction::EAST:
    case Direction::WEST:
        return CursorStyle::RESIZE_EW;
    case Direction::NORTH:
    case Direction::SOUTH:
        return CursorStyle::RESIZE_NS;
    case Direction::NORTH_WEST:
    case Direction::SOUTH_EAST:
        return CursorStyle::RESIZE_NWSE;
    default:
        return CursorStyle::RESIZE_NESW;
    }
}

export struct RequestCursorEvent {
    CursorStyle style;
};

export struct FormfactorEvent {
    FormFactor formFactor;
};

export struct DragStartEvent {
};

} // namespace Karm::App
