export module Karm.App.Base:hit;

namespace Karm::App {

export enum struct HitResult {
    NORMAL,
    HIT,
    DRAG,
    RESIZE_EAST,
    RESIZE_NORTH,
    RESIZE_NORTH_EAST,
    RESIZE_NORTH_WEST,
    RESIZE_SOUTH,
    RESIZE_SOUTH_EAST,
    RESIZE_SOUTH_WEST,
    RESIZE_WEST,
};

} // namespace Karm::App
