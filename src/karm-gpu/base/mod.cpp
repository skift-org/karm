export module Karm.Gpu.Base;

import Karm.Core;

namespace Karm::Gpu {

/// Winding order that determines which side of a triangle is front-facing.
export enum struct FrontFace : u8 {
    COUNTER_CLOCKWISE = 0,
    CLOCKWISE,
};

/// Which triangle faces are discarded during rasterization.
export enum struct Cull : u8 {
    FRONT,
    BACK,
    NONE,
};

/// Flags controlling whether the depth buffer is read and/or written.
export enum struct DepthFlags : u8 {
    NONE = 0,
    READ = 1 << 0,
    WRITE = 1 << 1,
};

/// Comparison operation for depth and stencil testing
export enum struct Op : u8 {
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS,
};

/// Operations for stencil buffers
export enum struct StencilOp : u8 {
    KEEP,
    ZERO,
    REPLACE,
    INCREMENT_CLAMP,
    DECREMENT_CLAMP,
    INVERT,
    INCREMENT_WRAP,
    DECREMENT_WRAP,
};

/// Operations for color/alpha blending.
export enum struct Blend : u8 {
    ADD,
    SUBTRACT,
    REV_SUBTRACT,
    MIN,
    MAX,
};

/// Blend factors for color/alpha blending.
export enum struct Factor : u8 {
    ZERO,
    ONE,
    SRC_COLOR,
    DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
};

/// Input primitive to be used for a render pass.
export enum struct Topology : u8 {
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
};

} // namespace Karm::Gpu
