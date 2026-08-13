#include <karm/entry>

import Karm.App;
import Karm.Core;
import Karm.Drm;
import Karm.Gfx.Pixels;
import Karm.Math;
import Karm.Sys;
import Karm.Image;
import Karm.Gpu.Base;
import Karm.Gpu.Rast;
import Karm.Scene3d;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Gears {

struct ShaderArgs {
    Scene3d::Mesh const& mesh;
    Gfx::Pixels texture;
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
    auto c = args->texture.sample(in->uv).vec4() * i;

    out[0] = {
        clamp(c.x, 0.0f, 1.0f),
        clamp(c.y, 0.0f, 1.0f),
        clamp(c.z, 0.0f, 1.0f),
        c.w,
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
    Scene3d::Mesh cube;
    Rc<Gfx::Image> image;
    Rc<App::Window> win;
    Rc<App::SwapChain> swapChain;
    Vec<f64> depths;
    float animation = 0;

    Handler(Rc<App::Window> win, Rc<Gfx::Image> image)
        : image(image), win(win), swapChain(win->createSwapChain().unwrap()) {

        depths.resize(swapChain->size.width * swapChain->size.height);
        cube = Scene3d::Mesh::cube(2);
    }

    void update() override {
        f64 dt = 1.0 / 60.0;
        animation += dt * 4;

        auto [buffer, _] = swapChain->acquire();
        auto pixels = Gfx::MutPixels::from(buffer);

        auto h = pixels.height() / (f64)pixels.width();
        Math::Mat4f t =
            Math::Mat4f::frustum(-1, 1, -h, h, 5, 40) *
            Math::Mat4f::translation(0, 0, -12);

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
            {0.2}
        };

        Gpu::Rasterizer::Pass pass;
        pass.attachments = {&attachment, 1};

        pass.depth.data = depths;
        pass.depth.loadOp = Gpu::LoadOp::CLEAR;
        pass.depth.storeOp = Gpu::StoreOp::STORE;
        pass.depth.clearColor = {1};
        Gpu::Rasterizer::beginPass(pass);

        static constexpr Math::Vec4f light = {5.0, 5.0, 10.0, 0.0};
        auto m = Math::Mat4f::rotationX(Math::PI / 6) * Math::Mat4f::rotationY(animation);
        ShaderArgs a{
            cube,
            image->pixels(),
            {(m.inverse() * light).xyz.unit(), 0},
            t * m,
        };

        Gpu::Rasterizer::drawPrimitives(
            state,
            pass,
            pipeline,
            &a,
            &a,
            cube.len(),
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
    auto img = co_try$(Image::load("bundle://gpu-cube/skift.png"_url));
    auto handler = makeRc<Gears::Handler>(win, img);
    co_return co_await app->runAsync(handler, ct);
}
