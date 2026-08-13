export module Karm.Scene3d:mesh;

import Karm.Core;
import Karm.Math;

namespace Karm::Scene3d {

export struct Vertex {
    Math::Vec3f position;
    Math::Vec3f normal;
    Math::Vec2f uv;
};

export struct Mesh {
    Vec<Math::Vec3f> positions;
    Vec<Math::Vec3f> normals;
    Vec<Math::Vec2f> uvs;
    Vec<isize> indexes;

    static Mesh plane(Math::Vec3f position, Math::Vec3f direction, f32 width, f32 height, usize widthSegments, usize heightSegments) {
        Mesh mesh;

        widthSegments = max(widthSegments, usize{1});
        heightSegments = max(heightSegments, usize{1});

        auto n = direction.unitOr({0, 0, 1});

        auto a = n.abs();
        Math::Vec3f ref = a.x <= a.y and a.x <= a.z ? Math::Vec3f{1, 0, 0}
                          : a.y <= a.z              ? Math::Vec3f{0, 1, 0}
                                                    : Math::Vec3f{0, 0, 1};

        auto u = ref.cross(n).unit();
        auto v = n.cross(u);

        Math::Vec2f size{width, height};
        Math::Vec2f segments{(f32)widthSegments, (f32)heightSegments};

        usize cols = widthSegments + 1;
        usize rows = heightSegments + 1;

        for (usize y = 0; y < rows; y++) {
            for (usize x = 0; x < cols; x++) {
                auto st = Math::Vec2u{x, y}.cast<f32>() / segments; // parametric, +y follows +v
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

    static Mesh cube(f32 size) {
        Mesh mesh;
        Array normals = {
            Math::Vec3f{1, 0, 0},
            Math::Vec3f{-1, 0, 0},
            Math::Vec3f{0, 1, 0},
            Math::Vec3f{0, -1, 0},
            Math::Vec3f{0, 0, 1},
            Math::Vec3f{0, 0, -1}
        };
        for (auto n : normals)
            mesh.extend(plane(n * (size / 2), n, size, size, 1, 1));
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

    void extend(Mesh const& other) {
        isize offset = positions.len();
        for (auto& p : other.positions)
            positions.pushBack(p);
        for (auto& n : other.normals)
            normals.pushBack(n);
        for (auto& uv : other.uvs)
            uvs.pushBack(uv);
        for (auto i : other.indexes)
            indexes.pushBack(i + offset);
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

} // namespace Karm::Scene3d