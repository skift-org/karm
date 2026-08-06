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
    NONE,
    BACK,
    FRONT,
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

export template <typename T>
bool compare(Op op, T lhs, T rhs) {
    switch (op) {
    case Op::NEVER:
        return false;
    case Op::LESS:
        return lhs < rhs;
    case Op::EQUAL:
        return lhs == rhs;
    case Op::LESS_EQUAL:
        return lhs <= rhs;
    case Op::GREATER:
        return lhs > rhs;
    case Op::NOT_EQUAL:
        return lhs != rhs;
    case Op::GREATER_EQUAL:
        return lhs >= rhs;
    case Op::ALWAYS:
        return true;
    }
}

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

/// How an attachment's contents are handled at the start of a render pass.
export enum struct LoadOp : u8 {
    UNDEFINED,
    LOAD,
    CLEAR,
};

/// How an attachment's contents are handled at the end of a render pass.
export enum struct StoreOp : u8 {
    UNDEFINED,
    STORE,
    DISCARD,
};

/// Per-face stencil test and the operations applied on its outcome.
export struct Stencil {
    Op test = Op::ALWAYS;
    StencilOp failOp = StencilOp::KEEP;
    StencilOp passOp = StencilOp::KEEP;
    StencilOp depthFailOp = StencilOp::KEEP;
    u8 reference = 0;
};

/// Description of the depth test, depth bias, and stencil behavior of a pipeline.
export struct DepthStencilProps {
    Flags<DepthFlags> depthMode = DepthFlags::NONE;
    Op depthTest = Op::ALWAYS;
    f32 depthBias = 0.0f;
    f32 depthBiasSlopeFactor = 0.0f;
    f32 depthBiasClamp = 0.0f;
    u8 stencilReadMask = 0xff;
    u8 stencilWriteMask = 0xff;
    Stencil stencilFront;
    Stencil stencilBack;
};

} // namespace Karm::Gpu
