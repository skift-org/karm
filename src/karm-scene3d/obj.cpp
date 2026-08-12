module;

#include <karm/macros>

export module Karm.Scene3d:obj;

import Karm.Core;
import Karm.Math;
import Karm.Logger;
import :mesh;

using namespace Karm::Re::Literals;

namespace Karm::Scene3d {

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

export Res<Mesh> loadObj(Io::SScan& s) {
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

} // namespace Karm::Scene3d