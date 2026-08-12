#include <karm/entry>

import Karm.App;
import Karm.Core;
import Karm.Drm;
import Karm.Gfx.Pixels;
import Karm.Math;
import Karm.Sys;
import Karm.Gpu.Base;
import Karm.Gpu.Rast;
import Karm.Scene3d;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Gears {

static Scene3d::Mesh gear(f64 innerRadius, f64 outerRadius, f64 width, u64 teeth, f64 toothDepth) {
    Scene3d::Mesh mesh;
    teeth = max(teeth, u64{1});

    f64 const r0 = innerRadius;
    f64 const r1 = outerRadius - toothDepth / 2.0;
    f64 const r2 = outerRadius + toothDepth / 2.0;
    f64 const hw = width * 0.5;
    f64 const da = Math::TAU / (f64)teeth / 4.0;

    auto polar = [](f64 r, f64 a, f64 z) -> Math::Vec3f {
        return {r * Math::cos(a), r * Math::sin(a), z};
    };

    auto frontUv = [&](Math::Vec3f p) -> Math::Vec2f {
        return {p.x / (2 * r2) + 0.5, p.y / (2 * r2) + 0.5};
    };

    auto backUv = [&](Math::Vec3f p) -> Math::Vec2f {
        return {0.5 - p.x / (2 * r2), p.y / (2 * r2) + 0.5};
    };

    auto rimUv = [&](f64 a, f64 z) -> Math::Vec2f {
        return {a / Math::TAU, (z + hw) / width};
    };

    auto tri = [&](Math::Vec3f n,
                   Math::Vec3f a, Math::Vec2f ua,
                   Math::Vec3f b, Math::Vec2f ub,
                   Math::Vec3f c, Math::Vec2f uc) {
        auto ia = mesh.vertex(a, n, ua);
        auto ib = mesh.vertex(b, n, ub);
        auto ic = mesh.vertex(c, n, uc);
        mesh.triangle(ia, ib, ic);
    };

    auto quad = [&](Math::Vec3f n,
                    Math::Vec3f a, Math::Vec2f ua,
                    Math::Vec3f b, Math::Vec2f ub,
                    Math::Vec3f c, Math::Vec2f uc,
                    Math::Vec3f d, Math::Vec2f ud) {
        auto ia = mesh.vertex(a, n, ua);
        auto ib = mesh.vertex(b, n, ub);
        auto ic = mesh.vertex(c, n, uc);
        auto id = mesh.vertex(d, n, ud);
        mesh.triangle(ia, ib, ic);
        mesh.triangle(ia, ic, id);
    };

    auto rim = [&](f64 aA, f64 rA, f64 aB, f64 rB, Math::Vec3f n) {
        quad(
            n,
            polar(rA, aA, hw), rimUv(aA, hw),
            polar(rA, aA, -hw), rimUv(aA, -hw),
            polar(rB, aB, -hw), rimUv(aB, -hw),
            polar(rB, aB, hw), rimUv(aB, hw)
        );
    };

    auto wallNormal = [](Math::Vec3f from, Math::Vec3f to) -> Math::Vec3f {
        auto d = to - from;
        return Math::Vec3f{d.y, -d.x, 0}.unit();
    };

    auto radial = [](f64 a) -> Math::Vec3f {
        return {Math::cos(a), Math::sin(a), 0};
    };

    Math::Vec3f const front{0, 0, 1};
    Math::Vec3f const back{0, 0, -1};

    for (u64 i = 0; i < teeth; i++) {
        f64 const a0 = (f64)i * 4 * da;
        f64 const a1 = a0 + da;
        f64 const a2 = a0 + 2 * da;
        f64 const a3 = a0 + 3 * da;
        f64 const a4 = a0 + 4 * da;

        auto h0 = polar(r0, a0, hw);
        auto h4 = polar(r0, a4, hw);
        auto b0 = polar(r1, a0, hw);
        auto b3 = polar(r1, a3, hw);
        auto b4 = polar(r1, a4, hw);
        auto t1 = polar(r2, a1, hw);
        auto t2 = polar(r2, a2, hw);

        tri(
            front,
            h0, frontUv(h0),
            b0, frontUv(b0),
            b3, frontUv(b3)
        );
        quad(
            front,
            h0, frontUv(h0),
            b3, frontUv(b3),
            b4, frontUv(b4),
            h4, frontUv(h4)
        );
        quad(
            front,
            b0, frontUv(b0),
            t1, frontUv(t1),
            t2, frontUv(t2),
            b3, frontUv(b3)
        );

        auto H0 = polar(r0, a0, -hw);
        auto H4 = polar(r0, a4, -hw);
        auto B0 = polar(r1, a0, -hw);
        auto B3 = polar(r1, a3, -hw);
        auto B4 = polar(r1, a4, -hw);
        auto T1 = polar(r2, a1, -hw);
        auto T2 = polar(r2, a2, -hw);

        tri(
            back,
            H0, backUv(H0),
            B3, backUv(B3),
            B0, backUv(B0)
        );
        quad(
            back,
            H0, backUv(H0),
            H4, backUv(H4),
            B4, backUv(B4),
            B3, backUv(B3)
        );
        quad(
            back,
            B0, backUv(B0),
            B3, backUv(B3),
            T2, backUv(T2),
            T1, backUv(T1)
        );

        rim(a0, r1, a1, r2, wallNormal(b0, t1));
        rim(a1, r2, a2, r2, radial(a0 + 1.5 * da));
        rim(a2, r2, a3, r1, wallNormal(t2, b3));
        rim(a3, r1, a4, r1, radial(a0 + 3.5 * da));
    }

    isize const base = (isize)mesh.positions.len();
    for (u64 i = 0; i <= teeth; i++) {
        f64 const a = (f64)i * Math::TAU / (f64)teeth;
        auto n = -radial(a); // faces the axis
        mesh.vertex(polar(r0, a, -hw), n, rimUv(a, -hw));
        mesh.vertex(polar(r0, a, hw), n, rimUv(a, hw));
    }
    for (u64 i = 0; i < teeth; i++) {
        isize const a = base + (isize)i * 2;
        isize const b = a + 1;
        isize const c = a + 2;
        isize const d = a + 3;
        mesh.triangle(a, b, c);
        mesh.triangle(c, b, d);
    }

    return mesh;
}

struct ShaderArgs {
    Scene3d::Mesh const& mesh;
    Gpu::Color color;
    Math::Vec4f light;
    Math::Mat4f local;
};

struct VertexData {
    Math::Vec4f position;
    Math::Vec3f normal;
    Math::Vec2f uv;
    Math::Vec3f local;
};

void vertMain(ShaderArgs* args, VertexData* out, Gpu::Rasterizer::VertexSystemValue* sv) {
    auto const& v = args->mesh[sv->vertexId];
    out->position = args->local * Math::Vec4f{v.position, 1};
    out->normal = v.normal;
    out->uv = v.uv;
    out->local = v.position;
}

void fragMain(ShaderArgs* args, Gpu::Color* out, VertexData* in, Gpu::Rasterizer::FragmentSystemValue*) {
    auto n = in->normal.unit();

    auto l = args->light.w == 0.0
                 ? args->light.xyz
                 : (args->light.xyz - in->local).unit();

    auto i = 0.2 + max(n.dot(l), 0.0);
    auto c = args->color * i;

    out[0] = {
        clamp(c.x, 0.0, 1.0),
        clamp(c.y, 0.0, 1.0),
        clamp(c.z, 0.0, 1.0),
        args->color.w,
    };
}

static Gpu::Rasterizer::Pipeline pipeline{
    .vertex = {
        .len = sizeof(VertexData) / sizeof(f64),
        .main = reinterpret_cast<Gpu::Rasterizer::VertexShaderBlob::Main>(vertMain),
    },
    .fragment = {
        .main = reinterpret_cast<Gpu::Rasterizer::FragmentShaderBlob::Main>(fragMain),
    }
};

struct Handler : App::Handler {
    Scene3d::Mesh gear0;
    Scene3d::Mesh gear1;
    Scene3d::Mesh gear2;
    Rc<App::Window> win;
    Rc<App::SwapChain> swapChain;
    Vec<f64> depths;
    float animation = 0;

    Handler(Rc<App::Window> win)
        : win(win), swapChain(win->createSwapChain().unwrap()) {

        depths.resize(swapChain->size.width * swapChain->size.height);
        gear0 = gear(1.0, 4.0, 1.0, 20, 0.7);
        gear1 = gear(0.5, 2.0, 2.0, 10, 0.7);
        gear2 = gear(1.3, 2.0, 0.5, 10, 0.7);
    }

    void update() override {
        f64 dt = 1.0 / 60.0;
        animation += dt;

        auto [buffer, _] = swapChain->acquire();
        auto pixels = Gfx::MutPixels::from(buffer);

        auto h = pixels.height() / (f64)pixels.width();
        auto rot = Math::Mat4f::rotationX(20 * Math::DEG2RAD) *
                   Math::Mat4f::rotationY(30 * Math::DEG2RAD);
        Math::Mat4f t =
            Math::Mat4f::frustum(-1, 1, -h, h, 5, 60) *
            Math::Mat4f::translation(0, 0, -40);

        Gpu::Rasterizer::State state;
        state.viewport = {
            .bound = {0, 0, (f64)pixels.width(), (f64)pixels.height()},
            .minDepth = 0,
            .maxDepth = 1,
        };
        state.frontFace = Gpu::FrontFace::CLOCKWISE;
        state.cullMode = Gpu::Cull::BACK;
        state.depthStencil.depthTest = Gpu::Op::LESS;
        state.depthStencil.depthMode = {Gpu::DepthFlags::READ, Gpu::DepthFlags::WRITE};

        Gpu::Rasterizer::Attachment attachment{
            pixels,
            Gpu::LoadOp::CLEAR,
            Gpu::StoreOp::STORE,
            Gfx::BLACK.vec4()
        };

        Gpu::Rasterizer::Pass pass;
        pass.attachments = {&attachment, 1};

        pass.depth.data = depths;
        pass.depth.loadOp = Gpu::LoadOp::CLEAR;
        pass.depth.storeOp = Gpu::StoreOp::STORE;
        pass.depth.clearColor = {1};
        Gpu::Rasterizer::beginPass(pass);

        static constexpr Math::Vec4f light = {5.0, 5.0, 10.0, 0.0};
        auto m = rot * Math::Mat4f::translation(-3.0, -2.0, 0.0) * Math::Mat4f::rotationZ(animation);
        ShaderArgs a{
            gear0,
            {0.8, 0.1, 0.0, 1.0},
            {(m.inverse() * light).xyz.unit(), 0},
            t * m,
        };
        Gpu::Rasterizer::drawPrimitives(
            state,
            pass,
            pipeline,
            &a,
            &a,
            gear0.len(),
            1,
            0,
            0
        );

        m = rot * Math::Mat4f::translation(3.1, -2.0, 0.0) * Math::Mat4f::rotationZ(-2.0 * animation - 9.0);
        ShaderArgs b{
            gear1,
            {0.0, 0.8, 0.2, 1.0},
            {(m.inverse() * light).xyz.unit(), 0},
            t * m,
        };
        Gpu::Rasterizer::drawPrimitives(
            state,
            pass,
            pipeline,
            &b,
            &b,
            gear1.len(),
            1,
            0,
            0
        );

        m = rot * Math::Mat4f::translation(-3.1, 4.2, 0.0) * Math::Mat4f::rotationZ(-2.0 * animation - 25.0);
        ShaderArgs c{
            gear2,
            {0.2, 0.2, 1.0, 1.0},
            {(m.inverse() * light).xyz.unit(), 0},
            t * m,
        };
        Gpu::Rasterizer::drawPrimitives(
            state,
            pass,
            pipeline,
            &c,
            &c,
            gear2.len(),
            1,
            0,
            0
        );

        swapChain->present(buffer);
    }

    void handle(App::WindowId, App::Event& e) override {
        if (e.is<App::ResizeEvent>()) {
            swapChain = win->createSwapChain().unwrap();
            depths.resize(swapChain->size.width * swapChain->size.height);
            e.accept();
        }
    }
};

} // namespace Gears

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto app = co_trya$(App::Application::createAsync(env, {}, ct));
    auto win = co_trya$(app->createWindowAsync({.size = {300, 300}}, ct));
    auto handler = makeRc<Gears::Handler>(win);
    co_return co_await app->runAsync(handler, ct);
}
