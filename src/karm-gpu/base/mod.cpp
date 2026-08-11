module;

#include <karm/macros>

export module Karm.Gpu.Base;

import Karm.Core;
import Karm.Math;

namespace Karm::Gpu {

/// An RGBA color value.
export using Color = Math::Vec4f;

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

// MARK: Topology & Backface ---------------------------------------------------

/// Input primitive to be used for a render pass.
export enum struct Topology : u8 {
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
};

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

// MARK: Depth & Stencil -------------------------------------------------------

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
always_inline bool compare(Op op, T lhs, T rhs) {
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

/// Flags controlling whether the depth buffer is read and/or written.
export enum struct DepthFlags : u8 {
    NONE = 0,
    READ = 1 << 0,
    WRITE = 1 << 1,
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

// MARK: Viewport & Scissor ----------------------------------------------------

export struct Viewport {
    Math::Rectf bound;
    f64 minDepth = 0;
    f64 maxDepth = 1;
};

// MARK: Sampler ---------------------------------------------------------------

/// Coordinate space used when sampling a texture.
export enum struct SamplerCoords : u8 {
    NORMALIZED, ///< Coordinates lie in [0,1] range
    PIXEL,      ///< Coordinates lie in [0, width] and [0, height] range

    _LEN,
};

/// Filtering applied when a texture is minified, magnified, or sampled between mips.
export enum struct SamplerFilter : u8 {
    NEAREST,
    LINEAR,

    _LEN,
};

/// How texture coordinates outside the [0,1] range are resolved.
export enum struct SamplerAddressing : u8 {
    CLAMP_TO_EDGE,
    REPEAT,
    MIRRORED,

    _LEN,
};

/// Description of how a sampler filters and addresses texture reads.
export struct SamplerProps {
    SamplerCoords coord = SamplerCoords::NORMALIZED;
    SamplerFilter filter = SamplerFilter::NEAREST;
    SamplerAddressing address = SamplerAddressing::CLAMP_TO_EDGE;
    f32 maxAnisotropy = 1.0f;
};

// MARK: Draw ------------------------------------------------------------------

/// Bit width of the indices in an index buffer.
export enum struct IndexType : u8 {
    U16,
    U32,

    _LEN,
};

/// GPU-side layout of the arguments consumed by an indirect indexed draw.
export struct DrawIndexedIndirectGpuArgs {
    u32 indexCount;
    u32 instanceCount;
    u32 firstIndex;
    i32 vertexOffset;
    u32 firstInstance;
};

} // namespace Karm::Gpu
