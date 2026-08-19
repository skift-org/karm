module;

#include <karm/macros>

export module Karm.Gpu.Rast;

import Karm.Core;
import Karm.Math;
import Karm.Gfx.Pixels;
import Karm.Gpu.Base;

namespace Karm::Gpu::Rasterizer {

static constexpr usize MAX_PER_VERTEX_DATA = 64;

export struct State {
    DepthStencilProps depthStencil = {};
    Viewport viewport;
    Opt<Math::Recti> scissor;
    FrontFace frontFace;
    Cull cullMode;
};

export struct Attachment {
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

export struct Depth {
    MutSlice<f64> data;
    LoadOp loadOp = LoadOp::CLEAR;
    StoreOp storeOp = StoreOp::STORE;
    Color clearColor = {};
};

export struct Stencil {
    MutSlice<u8> data;
    LoadOp loadOp = LoadOp::CLEAR;
    StoreOp storeOp = StoreOp::STORE;
    Color clearColor = {};
};

export struct Pass {
    MutSlice<Attachment> attachments;
    Depth depth;
    Stencil stencil;
};

export struct VertexSystemValue {
    u32 vertexId;
    u32 instanceId;
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

export struct VertexShaderBlob {
    static constexpr u32 MAGIC = 0x5E515E51;
    using Main = void (*)(void* args, f64* vertex, VertexSystemValue* sv);

    u32 magic = MAGIC;
    u32 flags = {};
    Array<Interpolation, MAX_PER_VERTEX_DATA> interp = {};
    u32 positionLocation = 0;
    u32 len = 4;
    Main main = nullptr;
};

export struct FragmentShaderBlob {
    static constexpr u32 MAGIC = 0xF4A6F4A6;
    using Main = void (*)(void* args, Color* target, f64* interpolated, FragmentSystemValue* sv);

    u32 magic = MAGIC;
    u32 flags = {};
    Main main = nullptr;
};

export struct Pipeline {
    VertexShaderBlob vertex;
    FragmentShaderBlob fragment;
};

struct Primitive {
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

export void beginPass(Pass& pass) {
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
    State& state,
    Pass& pass,
    Pipeline& pipeline,
    void* fragmentArgs,
    f64* a, f64* b, f64* c,
    Primitive& primitive
) {
    primitive.a = {
        a[pipeline.vertex.positionLocation + 0],
        a[pipeline.vertex.positionLocation + 1],
        a[pipeline.vertex.positionLocation + 2],
        a[pipeline.vertex.positionLocation + 3],
    };

    primitive.b = {
        b[pipeline.vertex.positionLocation + 0],
        b[pipeline.vertex.positionLocation + 1],
        b[pipeline.vertex.positionLocation + 2],
        b[pipeline.vertex.positionLocation + 3],
    };

    primitive.c = {
        c[pipeline.vertex.positionLocation + 0],
        c[pipeline.vertex.positionLocation + 1],
        c[pipeline.vertex.positionLocation + 2],
        c[pipeline.vertex.positionLocation + 3],
    };

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
    Array<f64, MAX_PER_VERTEX_DATA> interpolated = {};
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
            if (pass.depth.data and state.depthStencil.depthMode.has(DepthFlags::READ)) {
                auto& d = pass.depth.data[y * targetBound.width + x];

                // https://docs.vulkan.org/spec/latest/chapters/fragops.html#fragops-depth-comparison
                if (not depthStencilTest(state.depthStencil.depthTest, sv.depth, d))
                    continue;
            }

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
    State& state,
    Pass& pass,
    Pipeline& pipeline,
    void* vertexArgs,
    void* fragmentArgs,
    u32 vertexCount,
    u32 instanceCount,
    u32 firstVertex,
    u32 firstInstance
) {
    VertexSystemValue sv{};
    Array<f64, MAX_PER_VERTEX_DATA * 3> data = {};
    Primitive primitive;

    for (auto instanceId : Iota(firstInstance, firstInstance + instanceCount)) {
        sv.instanceId = instanceId;

        for (auto vertexId : Iota(firstVertex, firstVertex + vertexCount, 3u)) {
            sv.vertexId = vertexId;
            auto a = &data[pipeline.vertex.len * 0];
            pipeline.vertex.main(vertexArgs, a, &sv);

            sv.vertexId = vertexId + 1;
            auto b = &data[pipeline.vertex.len * 1];
            pipeline.vertex.main(vertexArgs, b, &sv);

            sv.vertexId = vertexId + 2;
            auto c = &data[pipeline.vertex.len * 2];
            pipeline.vertex.main(vertexArgs, c, &sv);

            fillPrimitives(
                state,
                pass,
                pipeline,
                fragmentArgs,
                a, b, c,
                primitive
            );
        }
    }
}

struct VertexCache {
    static constexpr usize SLOTS = 16;
    static constexpr u32 EMPTY = ~0u;

    Array<u32, SLOTS> tags = {};
    Array<f64, SLOTS * MAX_PER_VERTEX_DATA> data = {};

    void access(VertexShaderBlob& shader, void* args, f64* vertex, VertexSystemValue* sv) {
        auto slot = sv->vertexId % SLOTS;
        f64* entry = &data[slot * MAX_PER_VERTEX_DATA];

        if (tags[slot] != sv->vertexId) {
            shader.main(args, entry, sv);
            tags[slot] = sv->vertexId;
        }

        for (auto j : Iota(shader.len))
            vertex[j] = entry[j];
    }

    void flush() {
        for (auto& t : tags)
            t = EMPTY;
    }
};

export void drawIndexed(
    State& state,
    Pass& pass,
    Pipeline& pipeline,
    void* vertexArgs,
    void* fragmentArgs,
    void* indices,
    IndexType indexType,
    DrawIndexedIndirectGpuArgs& args
) {
    VertexCache cache;
    VertexSystemValue sv{};
    Array<f64, MAX_PER_VERTEX_DATA * 3> data;
    Primitive primitive;

    auto fetchId = [&](u32 index) {
        return indexType == IndexType::U32
                   ? static_cast<u32*>(indices)[index]
                   : static_cast<u16*>(indices)[index] + args.vertexOffset;
    };

    for (auto instanceId : Iota(args.firstInstance, args.firstInstance + args.instanceCount)) {
        cache.flush();
        sv.instanceId = instanceId;
        for (auto vertexIndex : Iota(args.firstIndex, args.firstIndex + args.indexCount, 3u)) {
            sv.vertexId = fetchId(vertexIndex + 0);
            auto a = &data[pipeline.vertex.len * 0];
            cache.access(pipeline.vertex, vertexArgs, a, &sv);

            sv.vertexId = fetchId(vertexIndex + 1);
            auto b = &data[pipeline.vertex.len * 1];
            cache.access(pipeline.vertex, vertexArgs, b, &sv);

            sv.vertexId = fetchId(vertexIndex + 2);
            auto c = &data[pipeline.vertex.len * 2];
            cache.access(pipeline.vertex, vertexArgs, c, &sv);

            fillPrimitives(
                state,
                pass,
                pipeline,
                fragmentArgs,
                a, b, c,
                primitive
            );
        }
    }
}

export void drawIndirect(
    State& state,
    Pass& pass,
    Pipeline& pipeline,
    void* vertexArgs,
    void* fragmentArgs,
    void* indices,
    IndexType indexType,
    Slice<DrawIndexedIndirectGpuArgs> args
) {
    for (auto arg : args)
        drawIndexed(
            state,
            pass,
            pipeline,
            vertexArgs,
            fragmentArgs,
            indices,
            indexType,
            arg
        );
}

} // namespace Karm::Gpu::Rasterizer
