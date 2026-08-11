#include <karm/entry>

import Karm.App;
import Karm.Core;
import Karm.Drm;
import Karm.Gfx.Pixels;
import Karm.Logger;
import Karm.Math;
import Karm.Ref;
import Karm.Sys;
import Karm.Gpu.Base;
import Karm.Gpu.Rast;
import Karm.Image;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;
using namespace Karm::Re::Literals;

namespace Example {

struct Vertex {
    Math::Vec3f position;
    Math::Vec3f normal;
    Math::Vec2f uv;
};

struct Mesh {
    Vec<Math::Vec3f> positions;
    Vec<Math::Vec3f> normals;
    Vec<Math::Vec2f> uvs;
    Vec<isize> indexes;

    static Mesh plane(Math::Vec3f position, Math::Vec3f direction, f64 width, f64 height, usize widthSegments, usize heightSegments) {
        Mesh mesh;

        widthSegments = Karm::max(widthSegments, usize{1});
        heightSegments = Karm::max(heightSegments, usize{1});

        auto n = direction.unitOr({0, 0, 1});

        auto a = n.abs();
        Math::Vec3f ref = a.x <= a.y and a.x <= a.z ? Math::Vec3f{1, 0, 0}
                          : a.y <= a.z              ? Math::Vec3f{0, 1, 0}
                                                    : Math::Vec3f{0, 0, 1};

        auto u = ref.cross(n).unit();
        auto v = n.cross(u);

        Math::Vec2f size{width, height};
        Math::Vec2f segments{(f64)widthSegments, (f64)heightSegments};

        usize cols = widthSegments + 1;
        usize rows = heightSegments + 1;

        for (usize y = 0; y < rows; y++) {
            for (usize x = 0; x < cols; x++) {
                auto st = Math::Vec2u{x, y}.cast<f64>() / segments; // parametric, +y follows +v
                auto offset = (st - 0.5) * size;
                mesh.vertex(position + u * offset.x + v * offset.y, n, {st.x, 1 - st.y});
            }
        }

        for (usize y = 0; y < heightSegments; y++) {
            for (usize x = 0; x < widthSegments; x++) {
                isize a = (isize)(y * cols + x);
                isize b = a + 1;
                isize c = a + (isize)cols;
                isize d = c + 1;

                mesh.triangle(a, b, d);
                mesh.triangle(a, d, c);
            }
        }

        return mesh;
    }

    isize vertex(Math::Vec3f position, Math::Vec3f normal, Math::Vec2f uv) {
        positions.pushBack(position);
        normals.pushBack(normal);
        uvs.pushBack(uv);
        return positions.len() - 1;
    }

    void triangle(isize a, isize b, isize c) {
        indexes.pushBack(a);
        indexes.pushBack(b);
        indexes.pushBack(c);
    }

    Vertex operator[](usize i) const {
        return {
            positions[indexes[i]],
            normals[indexes[i]],
            uvs[indexes[i]],
        };
    }

    usize len() const {
        return indexes.len();
    }
};

static constexpr auto RE_REST_OF_LINE = Re::untilAndConsume('\n'_re);

static Res<Tuple<isize, isize, isize>> _parseFaceTuple(Io::SScan& s) {
    isize v = 0, vt = 0, vn = 0;
    v = try$(Io::atoi(s).okOr(Error::invalidInput("expected vertex position index")));
    if (s.skip('/'_re))
        vt = try$(Io::atoi(s).okOr(Error::invalidInput("expected vertex normal index")));
    if (s.skip('/'_re))
        vn = try$(Io::atoi(s).okOr(Error::invalidInput("expected vertex texture index")));
    return Ok(Tuple{v, vt, vn});
}

Res<Mesh> loadObj(Io::SScan& s) {
    Vec<Math::Vec3f> positions;
    Vec<Math::Vec3f> normals;
    Vec<Math::Vec2f> uvs;

    Vec<isize> positionsIndexes;
    Vec<isize> normalsIndexes;
    Vec<isize> uvsIndexes;

    while (not s.ended()) {
        if (s.skip("# ")) {
            s.skip(RE_REST_OF_LINE);
        } else if (s.skip("v ")) {
            auto x = try$(Io::atof(s).okOr(Error::invalidInput("expected vertex x position")));
            s.skip(Re::blank());
            auto y = try$(Io::atof(s).okOr(Error::invalidInput("expected vertex y position")));
            s.skip(Re::blank());
            auto z = try$(Io::atof(s).okOr(Error::invalidInput("expected vertex z position")));
            s.skip(RE_REST_OF_LINE);
            positions.pushBack({x, y, z});
        } else if (s.skip("vn ")) {
            auto x = try$(Io::atof(s).okOr(Error::invalidInput("expected vertex x position")));
            s.skip(Re::blank());
            auto y = try$(Io::atof(s).okOr(Error::invalidInput("expected vertex y position")));
            s.skip(Re::blank());
            auto z = try$(Io::atof(s).okOr(Error::invalidInput("expected vertex z position")));
            s.skip(RE_REST_OF_LINE);
            normals.pushBack({x, y, z});
        } else if (s.skip("vt ")) {
            auto x = try$(Io::atof(s).okOr(Error::invalidInput("expected vertex x position")));
            s.skip(Re::blank());
            auto y = try$(Io::atof(s).okOr(Error::invalidInput("expected vertex y position")));
            s.skip(RE_REST_OF_LINE);
            uvs.pushBack({x, y});
        } else if (s.skip("f ")) {
            auto [v0, vt0, vn0] = try$(_parseFaceTuple(s));
            s.skip(Re::blank());
            auto [v1, vt1, vn1] = try$(_parseFaceTuple(s));
            s.skip(Re::blank());
            auto [v2, vt2, vn2] = try$(_parseFaceTuple(s));
            s.skip(RE_REST_OF_LINE);

            positionsIndexes.pushBack(v0 - 1);
            positionsIndexes.pushBack(v1 - 1);
            positionsIndexes.pushBack(v2 - 1);

            uvsIndexes.pushBack(vt0 - 1);
            uvsIndexes.pushBack(vt1 - 1);
            uvsIndexes.pushBack(vt2 - 1);

            normalsIndexes.pushBack(vn0 - 1);
            normalsIndexes.pushBack(vn1 - 1);
            normalsIndexes.pushBack(vn2 - 1);
        } else {
            auto line = s.token(RE_REST_OF_LINE);
            logWarn("ignored line {:#}", line);
        }
    }

    Mesh mesh;
    Map<Tuple<isize, isize, isize>, isize> cache;

    for (usize i = 0; i + 2 < positionsIndexes.len(); i += 3) {
        Array<isize, 3> corners{};

        for (usize j = 0; j < 3; ++j) {
            auto pi = positionsIndexes[i + j];
            auto ti = uvsIndexes[i + j];
            auto ni = normalsIndexes[i + j];
            auto key = Tuple{pi, ti, ni};

            if (auto found = cache.lookup(key)) {
                corners[j] = *found;
                continue;
            }

            auto index = mesh.vertex(
                positions.index(pi).unwrapOr(Math::Vec3f{0, 0, 0}),
                normals.index(ni).unwrapOr(Math::Vec3f{0, 0, 0}),
                uvs.index(ti).unwrapOr(Math::Vec2f{0, 0})
            );

            cache.put(key, index);
            corners[j] = index;
        }

        mesh.triangle(corners[0], corners[1], corners[2]);
    }

    return Ok(std::move(mesh));
}

struct ShaderArgs {
    Mesh const& mesh;
    Gfx::Pixels texture;
    Math::Mat4f const& local;
};

struct VertexData {
    Math::Vec4f position;
    Math::Vec3f normal;
    Math::Vec2f uv;
};

void vertMain(ShaderArgs* args, VertexData* out, Gpu::VertexSystemValue* sv) {
    auto const& v = args->mesh[sv->vertexId];
    out->position = args->local * Math::Vec4f{v.position, 1};
    out->normal = v.normal;
    out->uv = v.uv;
}

void fragMain(ShaderArgs* args, Gpu::Color* out, VertexData* in, Gpu::FragmentSystemValue*) {
    out[0] = args->texture.sample(in->uv).vec4();
}

static Gpu::RasterizerPipeline pipeline{
    .vertex = {
        .len = sizeof(VertexData) / sizeof(f64),
        .main = reinterpret_cast<Gpu::RasterizerVertexShaderBlob::Main>(vertMain),
    },
    .fragment = {
        .main = reinterpret_cast<Gpu::RasterizerFragmentShaderBlob::Main>(fragMain),
    }
};

struct Camera {
    Math::Vec3f position{0, 0, 0};
    f64 yaw = 0;
    f64 pitch = 0;

    f64 speed = 5.0;
    f64 sensitivity = 0.004;

    bool goForward = false, goBack = false;
    bool goLeft = false, goRight = false;
    bool goUp = false, goDown = false;

    // Camera looks along -Z when yaw and pitch are 0
    Math::Vec3f forward() const {
        return {
            -Math::sin(yaw) * Math::cos(pitch),
            Math::sin(pitch),
            -Math::cos(yaw) * Math::cos(pitch),
        };
    }

    Math::Vec3f right() const {
        return {Math::cos(yaw), 0, -Math::sin(yaw)};
    }

    void key(App::Key k, bool pressed) {
        switch (k.code()) {
        case App::Key::W:
            goForward = pressed;
            break;
        case App::Key::S:
            goBack = pressed;
            break;
        case App::Key::A:
            goLeft = pressed;
            break;
        case App::Key::D:
            goRight = pressed;
            break;
        case App::Key::SPACE:
            goUp = pressed;
            break;
        case App::Key::LSHIFT:
        case App::Key::RSHIFT:
            goDown = pressed;
            break;
        default:
            break;
        }
    }

    void look(f64 dx, f64 dy) {
        yaw -= dx * sensitivity;
        pitch -= dy * sensitivity;

        // Do not let the camera turn over the poles
        f64 limit = Math::PI / 2 - 0.001;
        pitch = clamp(pitch, -limit, limit);
    }

    void update(f64 dt) {
        auto f = forward();
        auto r = right();

        Math::Vec3f dir{0, 0, 0};
        if (goForward)
            dir = dir + f;
        if (goBack)
            dir = dir - f;
        if (goRight)
            dir = dir + r;
        if (goLeft)
            dir = dir - r;
        if (goUp)
            dir.y += 1;
        if (goDown)
            dir.y -= 1;

        f64 len = Math::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len < 0.0001)
            return;

        f64 step = speed * dt / len;
        position = position + dir * step;
    }

    // The view matrix is the inverse of the camera transform
    Math::Mat4f view() const {
        return Math::Mat4f::rotationX(-pitch) *
               Math::Mat4f::rotationY(-yaw) *
               Math::Mat4f::translation(-position.x, -position.y, -position.z);
    }
};

struct Handler : App::Handler {
    Mesh model;
    Mesh ground;
    Opt<Rc<Gfx::Image>> grass;
    Opt<Rc<Gfx::Image>> stone;
    Rc<App::Window> win;
    Rc<App::SwapChain> swapChain;
    Vec<f64> depths;
    float animation = 0;

    Camera camera;
    Math::Vec3f position;

    Handler(Mesh model, Rc<App::Window> win)
        : model(std::move(model)), win(win), swapChain(win->createSwapChain().unwrap()) {
        camera.position = {0, 0, 10};
        ground = Mesh::plane(
            {0.f, 0.f, 0.f},
            {0.f, 1.f, 0.f},
            100.0,
            100.0,
            16,
            16
        );

        grass = Image::load("bundle://hello-app/uv-texture.png"_url).ok();
        stone = Image::load("bundle://hello-app/uv-texture.png"_url).ok();
    }

    void update() override {
        f64 dt = 1.0 / 60.0; // replace with the real frame time
        camera.update(dt);

        animation += 0.01;
        auto [buffer, _] = swapChain->acquire();
        auto pixels = Gfx::MutPixels::from(buffer);
        depths.resize(pixels.width() * pixels.height(), 1);

        Math::Mat4f t =
            Math::Mat4f::perspective(Math::PI / 2, pixels.width() / (f64)pixels.height(), 0.1, 1000) *
            camera.view();

        Gpu::RasterizerState state;
        state.viewport = {
            .bound = {0, 0, (f64)pixels.width(), (f64)pixels.height()},
            .minDepth = 0,
            .maxDepth = 1,
        };
        state.frontFace = Gpu::FrontFace::CLOCKWISE;
        state.cullMode = Gpu::Cull::BACK;
        state.depthStencil.depthTest = Gpu::Op::LESS;
        state.depthStencil.depthMode = {Gpu::DepthFlags::READ, Gpu::DepthFlags::WRITE};

        Gpu::RasterizerAttachment attachment{
            pixels,
            Gpu::LoadOp::CLEAR,
            Gpu::StoreOp::STORE,
            Gfx::BLUE800.vec4()
        };

        Gpu::RasterizerPass pass;
        pass.attachments = {&attachment, 1};

        pass.depth.data = depths;
        pass.depth.loadOp = Gpu::LoadOp::CLEAR;
        pass.depth.storeOp = Gpu::StoreOp::STORE;
        pass.depth.clearColor = {1};

        ShaderArgs a{model, stone.unwrap()->pixels(), t};
        Gpu::beginPass(pass);
        Gpu::drawPrimitives(
            state,
            pass,
            pipeline,
            &a,
            &a,
            model.len(),
            1,
            0,
            0
        );

        ShaderArgs b{ground, stone.unwrap()->pixels(), t};

        Gpu::drawPrimitives(
            state,
            pass,
            pipeline,
            &b,
            &b,
            ground.len(),
            1,
            0,
            0

        );

        swapChain->present(buffer);
    }

    void handle(App::WindowId, App::Event& e) override {
        if (e.is<App::ResizeEvent>()) {

            swapChain = win->createSwapChain().unwrap();
        } else if (auto ke = e.is<App::KeyboardEvent>()) {
            if (ke->type != App::KeyboardEvent::REPEATE)
                camera.key(ke->code, ke->type == App::KeyboardEvent::PRESS);
        } else if (auto me = e.is<App::MouseEvent>()) {
            if (me->type == App::MouseEvent::MOVE)
                camera.look(me->delta.x, me->delta.y);
        }
        e.accept();
    }
};

} // namespace Example

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken ct) {
    auto objData = co_try$(Sys::readAllText<Utf8>("bundle://hello-app/teapot.obj"_url));
    Io::SScan s{objData};
    auto objModel = co_try$(Example::loadObj(s));

    auto app = co_trya$(App::Application::createAsync(env, {}, ct));
    auto win = co_trya$(app->createWindowAsync({}, ct));
    auto handler = makeRc<Example::Handler>(std::move(objModel), win);
    co_return co_await app->runAsync(handler, ct);
}
