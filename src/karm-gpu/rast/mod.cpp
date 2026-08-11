module;

#include <karm/macros>

export module Karm.Gpu.Rast;

import Karm.Core;
import Karm.Math;
import Karm.Gfx.Pixels;
import Karm.Gpu.Base;

namespace Karm::Gpu {

export struct RasterizerState {
    DepthStencilProps depthStencil = {};
    Viewport viewport;
    Opt<Math::Recti> scissor;
    FrontFace frontFace;
    Cull cullMode;
};

export struct RasterizerAttachment {
    Gfx::MutPixels data = {
        nullptr,
        0,
        0,
        Gfx::RGBA8888
    };
    LoadOp loadOp = LoadOp::CLEAR;
    StoreOp storeOp = StoreOp::STORE;
    Color clearColor = {};
};

export struct RasterizerDepth {
    MutSlice<f64> data;
    LoadOp loadOp = LoadOp::CLEAR;
    StoreOp storeOp = StoreOp::STORE;
    Color clearColor = {};
};

export struct RasterizerStencil {
    MutSlice<u8> data;
    LoadOp loadOp = LoadOp::CLEAR;
    StoreOp storeOp = StoreOp::STORE;
    Color clearColor = {};
};

export struct RasterizerPass {
    MutSlice<RasterizerAttachment> attachments;
    RasterizerDepth depth;
    RasterizerStencil stencil;
};

export struct VertexSystemValue {
    usize vertexId;
    usize instanceId;
};

export struct FragmentSystemValue {
    f64 depth;
    Array<Math::Vec4f, 8> target;
    bool discard;
};

enum struct Interpolation : u8 {
    PERSPECTIVE,
    LINEAR,
    FLAT,
};

export struct RasterizerVertexShaderBlob {
    static constexpr u32 MAGIC = 0x5E515E51;
    using Main = void (*)(void* args, f64* vertex, VertexSystemValue* sv);

    u32 magic = MAGIC;
    usize flags = {};
    Array<Interpolation, 64> interp = {};
    u32 positionLocation = 0;
    usize len = 4;
    Main main = nullptr;
};

export struct RasterizerFragmentShaderBlob {
    static constexpr u32 MAGIC = 0xF4A6F4A6;
    using Main = void (*)(void* args, Color* target, f64* interpolated, FragmentSystemValue* sv);

    u32 magic = MAGIC;
    usize flags = {};
    Main main = nullptr;
};

export struct RasterizerPipeline {
    RasterizerVertexShaderBlob vertex;
    RasterizerFragmentShaderBlob fragment;
};

struct RasterizerPrimitive {
    Math::Vec4f a, b, c;

    always_inline constexpr Math::Tri2f xy() const {
        return Math::Tri2f(
            a.xy,
            b.xy,
            c.xy
        );
    }

    always_inline constexpr Math::Vec4f min() const {
        return a.min(b).min(c);
    }

    always_inline constexpr Math::Vec4f max() const {
        return a.max(b).max(c);
    }

    always_inline constexpr f64 xySignedArea() const {
        return xy().signedArea();
    }

    always_inline constexpr Math::Rectf xyBound() const {
        return Math::Rectf::fromTwoPoint(min().xy, max().xy);
    }

    always_inline constexpr Math::Vec3f xyBarycentricCoordinates(Math::Vec2f p) const {
        return {
            Math::Tri2f{p, b.xy, c.xy}.signedArea(),
            Math::Tri2f{p, c.xy, a.xy}.signedArea(),
            Math::Tri2f{p, a.xy, b.xy}.signedArea(),
        };
    }

    always_inline bool cull(FrontFace frontFace, Cull cull) {
        if (cull == Gpu::Cull::NONE)
            return false;

        auto area = (frontFace == Gpu::FrontFace::COUNTER_CLOCKWISE ? 1 : -1) * xySignedArea();
        if ((cull == Gpu::Cull::BACK and area < 0) or
            (cull == Gpu::Cull::FRONT and area > 0))
            return true;

        return false;
    }
};

export void beginPass(RasterizerPass& pass) {
    for (auto& a : pass.attachments) {
        if (a.loadOp == LoadOp::CLEAR)
            a.data.clear(Gfx::Color::fromFloats(a.clearColor));
    }

    if (pass.depth.data and pass.depth.loadOp == LoadOp::CLEAR) {
        for (auto& d : pass.depth.data)
            d = pass.depth.clearColor.r;
    }

    if (pass.stencil.data and pass.stencil.loadOp == LoadOp::CLEAR) {
        for (auto& d : pass.stencil.data)
            d = 255 * pass.stencil.clearColor.r;
    }
}

export void fillPrimitives(
    RasterizerState& state,
    RasterizerPass& pass,
    RasterizerPipeline& pipeline,
    void* fragmentArgs,
    f64* vertexData,
    RasterizerPrimitive& primitive
) {
    constexpr f64 NEAR_EPSILON = 1e-6;
    if (primitive.a.w <= NEAR_EPSILON or
        primitive.b.w <= NEAR_EPSILON or
        primitive.c.w <= NEAR_EPSILON)
        return;

    Math::Vec3f invW = {1 / primitive.a.w, 1 / primitive.b.w, 1 / primitive.c.w};

    primitive.a = primitive.a / primitive.a.w;
    primitive.b = primitive.b / primitive.b.w;
    primitive.c = primitive.c / primitive.c.w;

    // https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-viewport
    primitive.a.x = (state.viewport.bound.width / 2) * primitive.a.x + state.viewport.bound.center().x;
    primitive.a.y = (state.viewport.bound.height / 2) * primitive.a.y + state.viewport.bound.center().y;
    primitive.a.z = (state.viewport.maxDepth - state.viewport.minDepth) * primitive.a.z + state.viewport.minDepth;

    primitive.b.x = (state.viewport.bound.width / 2) * primitive.b.x + state.viewport.bound.center().x;
    primitive.b.y = (state.viewport.bound.height / 2) * primitive.b.y + state.viewport.bound.center().y;
    primitive.b.z = (state.viewport.maxDepth - state.viewport.minDepth) * primitive.b.z + state.viewport.minDepth;

    primitive.c.x = (state.viewport.bound.width / 2) * primitive.c.x + state.viewport.bound.center().x;
    primitive.c.y = (state.viewport.bound.height / 2) * primitive.c.y + state.viewport.bound.center().y;
    primitive.c.z = (state.viewport.maxDepth - state.viewport.minDepth) * primitive.c.z + state.viewport.minDepth;

    auto targetBound = pass.attachments[0].data.bound();
    auto primitiveBound = primitive.xyBound().ceil().cast<isize>();

    if (not targetBound.collide(primitiveBound))
        return;
    primitiveBound = primitiveBound.clipTo(targetBound);

    // https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-polygons-basic
    if (primitive.cull(state.frontFace, state.cullMode))
        return;

    auto invArena = 1 / primitive.xySignedArea();

    FragmentSystemValue sv = {};
    Array<f64, 64> interpolated = {};
    Array<Color, 8> targets = {};

    for (auto y : irange::fromStartEnd(primitiveBound.top(), primitiveBound.bottom())) {
        for (auto x : irange::fromStartEnd(primitiveBound.start(), primitiveBound.end())) {
            Math::Vec2i p = {x, y};
            auto coords = primitive.xyBarycentricCoordinates(p.cast<f64>()) * invArena;
            if (coords.x < 0 or coords.y < 0 or coords.z < 0)
                continue;

            sv.depth = primitive.a.z * coords.x +
                       primitive.b.z * coords.y +
                       primitive.c.z * coords.z;

            auto coordsPerspective = [&] -> Math::Vec3f {
                auto w = coords * invW;
                return w * (1.0 / (w.x + w.y + w.z));
            }();

            // https://docs.vulkan.org/spec/latest/chapters/fragops.html#fragops-depth
            if (pass.depth.data and state.depthStencil.depthMode.has(Gpu::DepthFlags::READ)) {
                auto& d = pass.depth.data[y * targetBound.width + x];

                // https://docs.vulkan.org/spec/latest/chapters/fragops.html#fragops-depth-comparison
                if (not Gpu::compare(state.depthStencil.depthTest, sv.depth, d))
                    continue;
            }

            f64* a = vertexData + pipeline.vertex.len * 0;
            f64* b = vertexData + pipeline.vertex.len * 1;
            f64* c = vertexData + pipeline.vertex.len * 2;

            for (auto i : Iota(pipeline.vertex.len)) {
                switch (pipeline.vertex.interp[i]) {
                case Interpolation::PERSPECTIVE:
                    interpolated[i] = a[i] * coordsPerspective.x + b[i] * coordsPerspective.y + c[i] * coordsPerspective.z;
                    break;
                case Interpolation::LINEAR:
                    interpolated[i] = a[i] * coords.x + b[i] * coords.y + c[i] * coords.z;
                    break;
                case Interpolation::FLAT:
                    interpolated[i] = a[i];
                    break;
                }
            }

            sv.discard = false;

            pipeline.fragment.main(fragmentArgs, targets.buf(), interpolated.buf(), &sv);

            if (sv.discard) [[unlikely]]
                continue;

            if (pass.depth.data and
                state.depthStencil.depthMode.has(Gpu::DepthFlags::WRITE) and
                pass.depth.storeOp == StoreOp::STORE)
                pass.depth.data[y * targetBound.width + x] = sv.depth;

            for (auto i : Iota(pass.attachments.len())) {
                auto& attachment = pass.attachments[i];

                if (attachment.storeOp == StoreOp::STORE)
                    attachment.data.storeUnsafe(p, Gfx::Color::fromFloats(targets[i]));
            }
        }
    }
}

export void drawPrimitives(
    RasterizerState& state,
    RasterizerPass& pass,
    RasterizerPipeline& pipeline,
    void* vertexArgs,
    void* fragmentArgs,
    usize vertexCount,
    usize instanceCount,
    usize firstVertex,
    usize firstInstance
) {
    VertexSystemValue sv{};
    Array<f64, 64 * 3> data = {};
    RasterizerPrimitive primitive;

    for (auto instanceId : urange::fromStartEnd(firstInstance, firstInstance + instanceCount)) {
        sv.instanceId = instanceId;

        for (auto vertexId : Iota<usize>(firstVertex, firstVertex + vertexCount, 3)) {
            sv.vertexId = vertexId;

            pipeline.vertex.main(vertexArgs, &data[pipeline.vertex.len * 0], &sv);
            primitive.a = {
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 0 + 0],
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 0 + 1],
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 0 + 2],
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 0 + 3],
            };

            sv.vertexId = vertexId + 1;
            pipeline.vertex.main(vertexArgs, &data[pipeline.vertex.len * 1], &sv);
            primitive.b = {
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 1 + 0],
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 1 + 1],
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 1 + 2],
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 1 + 3],
            };

            sv.vertexId = vertexId + 2;
            pipeline.vertex.main(vertexArgs, &data[pipeline.vertex.len * 2], &sv);
            primitive.c = {
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 2 + 0],
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 2 + 1],
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 2 + 2],
                data[pipeline.vertex.positionLocation + pipeline.vertex.len * 2 + 3],
            };

            fillPrimitives(
                state,
                pass,
                pipeline,
                fragmentArgs,
                &data[pipeline.vertex.len * 0],
                primitive
            );
        }
    }
}

// export void drawIndexed(
//     RasterizerState& state,
//     RasterizerPass& pass,
//     RasterizerPipeline& pipeline,
//     void* vertexArgs,
//     void* fragmentArgs,
//     void* indices,
//     IndexType indexType,
//     DrawIndexedIndirectGpuArgs& args
// ) {
//     for (auto instanceId : urange::fromStartEnd(args.firstInstance, args.instanceCount)) {
//         for (auto vertexId : Iota<usize>(args.firstInstance, args.instanceCount, 3)) {
//         }
//     }
// }
//
// export void drawIndirect(
//     RasterizerState& state,
//     RasterizerPass& pass,
//     RasterizerPipeline& pipeline,
//     void* vertexArgs,
//     void* fragmentArgs,
//     void* indices,
//     IndexType indexType,
//     Slice<DrawIndexedIndirectGpuArgs> args
// ) {
//     for (auto arg : args)
//         drawIndexed(
//             state,
//             pass,
//             pipeline,
//             vertexArgs,
//             fragmentArgs,
//             indices,
//             indexType,
//             arg
//         );
// }

} // namespace Karm::Gpu
