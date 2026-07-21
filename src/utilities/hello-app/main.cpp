#include <karm/entry>

import Karm.App;
import Karm.Core;
import Karm.Drm;
import Karm.Gfx.Pixels;
import Karm.Logger;
import Karm.Math;
import Karm.Ref;
import Karm.Sys;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;
using namespace Karm::Re::Literals;

namespace Example {

struct Triangle {
    Math::Vec3f a, b, c;

    constexpr Math::Tri2f xy() const {
        return Math::Tri2f(
            a.xy,
            b.xy,
            c.xy
        );
    }

    constexpr Math::Vec3f min() const {
        return a.min(b).min(c);
    }

    constexpr Math::Vec3f max() const {
        return a.max(b).max(c);
    }

    constexpr f64 xySignedArea() const {
        return xy().signedArea();
    }

    constexpr Math::Rectf xyBound() const {
        return Math::Rectf::fromTwoPoint(min().xy, max().xy);
    }

    constexpr Math::Vec3f xyBarycentricCoordinates(Math::Vec2f p) const {
        auto area = xySignedArea();
        return {
            Math::Tri2f{p, b.xy, c.xy}.signedArea() / area,
            Math::Tri2f{p, c.xy, a.xy}.signedArea() / area,
            Math::Tri2f{p, a.xy, b.xy}.signedArea() / area,
        };
    }
};

template <typename D>
struct PrimitiveData {
    D a, b, c;

    template <auto D::* Field>
    auto loadInterpolated(Math::Vec3f coords) const {
        return a.*Field * coords.x + b.*Field * coords.y + c.*Field * coords.z;
    }

    template <auto D::* Field>
    auto loadInterpolatedNoPerspective(Math::Vec3f coords) const {
        return a.*Field * coords.x + b.*Field * coords.y + c.*Field * coords.z;
    }

    template <auto D::* Field>
    auto loadFlat() const {
        return a.*Field;
    }

    Triangle triangle() const {
        return {
            a.position,
            b.position,
            c.position,
        };
    }
};

struct VertexData {
    Math::Vec3f position;
    Math::Vec3f normal;
    Math::Vec2f uv;

    static VertexData interpolate(PrimitiveData<VertexData> const& primitive, Math::Vec3f coords) {
        return {
            primitive.loadInterpolated<&VertexData::position>(coords),
            primitive.loadInterpolated<&VertexData::normal>(coords),
            primitive.loadInterpolated<&VertexData::uv>(coords),
        };
    }
};

struct FragmentData {
    Math::Vec4f color;
    f64 depth;
    bool discard = false;
};

struct Mesh {
    Vec<Math::Vec3f> positions;
    Vec<Math::Vec3f> normals;
    Vec<Math::Vec2f> uvs;
    Vec<isize> indexes;

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

    VertexData operator[](usize i) const {
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

struct Pipeline {
    Mesh& _mesh;
    Math::Mat4f& _local;

    VertexData vertex(usize vertexId) const {
        auto v = _mesh[vertexId];
        v.position = (_local * v.position).xyz;
        return v;
    }

    FragmentData fragment(VertexData data) const {
        return {
            .color = Math::Vec4f{data.normal * 0.5 + 0.5, 1},
            .depth = data.position.z,
            .discard = false
        };
    }
};

void drawPrimitive(Gfx::MutPixels px, MutSlice<f64> depth, Pipeline const& pipeline, PrimitiveData<VertexData> primitive) {
    Triangle triangle = primitive.triangle();
    auto bound = triangle.xyBound().ceil().cast<isize>();
    if (not px.bound().collide(bound))
        return;
    bound = bound.clipTo(px.bound());

    if (triangle.xySignedArea() < 1)
        return;

    for (auto y : irange::fromStartEnd(bound.top(), bound.bottom())) {
        for (auto x : irange::fromStartEnd(bound.start(), bound.end())) {
            Math::Vec2i p = {x, y};
            auto coords = triangle.xyBarycentricCoordinates(p.cast<f64>());
            if (coords.x < 0 or coords.y < 0 or coords.z < 0)
                continue;

            auto [color, z, discard] = pipeline.fragment(VertexData::interpolate(primitive, coords));
            if (discard)
                continue;
            if (depth) {
                auto& d = depth[y * px.width() + x];
                if (d > z)
                    continue;
                d = z;
            }
            px.store(p, Gfx::Color::fromFloats(color));
        }
    }
}

void draw(Gfx::MutPixels pixels, MutSlice<f64> depths, Pipeline const& pipeline, usize vertexCount) {
    for (auto vertexId : Iota<isize>(0, vertexCount, 3)) {
        PrimitiveData primitive{
            pipeline.vertex(vertexId + 0),
            pipeline.vertex(vertexId + 1),
            pipeline.vertex(vertexId + 2),
        };

        drawPrimitive(
            pixels,
            depths,
            pipeline,
            primitive
        );
    }
}

struct Handler : App::Handler {
    Mesh model;
    Rc<App::Window> win;
    Rc<App::SwapChain> swapChain;
    float rotate = 0;

    Handler(Mesh model, Rc<App::Window> win)
        : model(std::move(model)), win(win), swapChain(win->createSwapChain().unwrap()) {}

    void update() override {
        rotate += 0.01;
        auto [buffer, _] = swapChain->acquire();
        auto pixels = Gfx::MutPixels::from(buffer);
        pixels.clear(Gfx::BLUE800);
        Vec<f64> depths = {};
        depths.resize(pixels.width() * pixels.height(), -1000);

        Math::Mat4f t =
            Math::Mat4f::translation(pixels.width() / 2, pixels.height() / 2, 200) *
            Math::Mat4f::scaling(200, 200, 50) *
            Math::Mat4f::rotationY(rotate) *
            Math::Mat4f::rotationX(rotate);

        Pipeline p{model, t};
        draw(pixels, depths, p, model.len());
        swapChain->present(buffer);
    }

    void handle(App::WindowId, App::Event& e) override {
        if (e.is<App::ResizeEvent>())
            swapChain = win->createSwapChain().unwrap();
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
