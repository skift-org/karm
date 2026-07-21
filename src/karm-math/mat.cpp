export module Karm.Math:mat;

import :vec;

namespace Karm::Math {

// Matrices types, column major, post multiply

export template <typename T>
union Mat2 {
    using Scalar = T;

    struct {
        T m00, m01;
        T m10, m11;
    };

    struct {
        Vec2<T> col0;
        Vec2<T> col1;
    };

    Array<Vec2<T>, 2> cols;
    Array<T, 2 * 2> _els;

    static constexpr Mat2 identity() {
        return {
            {1, 0},
            {0, 1}
        };
    }

    Mat2() = default;

    Mat2(T m00, T m01, T m10, T m11)
        : _els{m00, m01, m10, m11} {}

    Mat2(Vec2<T> col0, Vec2<T> col1)
        : cols{col0, col1} {}

    void repr(Io::Emit& e) const {
        e("(mat2 ");
        for (auto& col : cols) {
            e("{}", col);
        }
        e(")");
    }
};

export using Mat2i = Mat2<i64>;
export using Mat2f = Mat2<f64>;

export template <typename T>
union Mat3 {
    using Scalar = T;

    struct {
        T m00, m01, m02;
        T m10, m11, m12;
        T m20, m21, m22;
    };

    struct {
        Vec3<T> col0;
        Vec3<T> col1;
        Vec3<T> col2;
    };

    Array<Vec3<T>, 3> cols;
    Array<T, 3 * 3> _els;

    Mat3() = default;

    Mat3(T m00, T m01, T m02, T m10, T m11, T m12, T m20, T m21, T m22)
        : _els{m00, m01, m02, m10, m11, m12, m20, m21, m22} {}

    Mat3(Vec3<T> col0, Vec3<T> col1, Vec3<T> col2)
        : cols{col0, col1, col2} {}

    void repr(Io::Emit& e) const {
        e("(mat3 ");
        for (auto& col : cols) {
            e("{}", col);
        }
        e(")");
    }
};

export using Mat3i = Mat3<i64>;
export using Mat3f = Mat3<f64>;

export template <typename T>
union Mat4 {
    using Scalar = T;

    struct {
        T m00, m01, m02, m03;
        T m10, m11, m12, m13;
        T m20, m21, m22, m23;
        T m30, m31, m32, m33;
    };

    struct {
        Vec4<T> col0;
        Vec4<T> col1;
        Vec4<T> col2;
        Vec4<T> col3;
    };

    Array<Vec4<T>, 4> cols;
    Array<T, 4 * 4> _els;

    Mat4() = default;

    Mat4(T m00, T m01, T m02, T m03, T m10, T m11, T m12, T m13, T m20, T m21, T m22, T m23, T m30, T m31, T m32, T m33)
        : _els{m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33} {}

    Mat4(Vec4<T> col0, Vec4<T> col1, Vec4<T> col2, Vec4<T> col3)
        : cols{col0, col1, col2, col3} {}

    static Mat4 identity() {
        return Mat4(
            Vec4<T>(1, 0, 0, 0),
            Vec4<T>(0, 1, 0, 0),
            Vec4<T>(0, 0, 1, 0),
            Vec4<T>(0, 0, 0, 1)
        );
    }

    static Mat4 zero() {
        return Mat4(
            Vec4<T>(0, 0, 0, 0),
            Vec4<T>(0, 0, 0, 0),
            Vec4<T>(0, 0, 0, 0),
            Vec4<T>(0, 0, 0, 0)
        );
    }

    static Mat4 translation(T x, T y, T z) {
        return Mat4(
            Vec4<T>(1, 0, 0, 0),
            Vec4<T>(0, 1, 0, 0),
            Vec4<T>(0, 0, 1, 0),
            Vec4<T>(x, y, z, 1)
        );
    }

    static Mat4 translation(Vec3<T> const& t) {
        return translation(t.x, t.y, t.z);
    }

    static Mat4 scaling(T x, T y, T z) {
        return Mat4(
            Vec4<T>(x, 0, 0, 0),
            Vec4<T>(0, y, 0, 0),
            Vec4<T>(0, 0, z, 0),
            Vec4<T>(0, 0, 0, 1)
        );
    }

    static Mat4 scaling(Vec3<T> const& s) {
        return scaling(s.x, s.y, s.z);
    }

    static Mat4 scaling(T uniform) {
        return scaling(uniform, uniform, uniform);
    }

    static Mat4 rotationX(T radians) {
        T c = cos(radians), s = sin(radians);
        return Mat4(
            Vec4<T>(1, 0, 0, 0),
            Vec4<T>(0, c, s, 0),
            Vec4<T>(0, -s, c, 0),
            Vec4<T>(0, 0, 0, 1)
        );
    }

    static Mat4 rotationY(T radians) {
        T c = cos(radians), s = sin(radians);
        return Mat4(
            Vec4<T>(c, 0, -s, 0),
            Vec4<T>(0, 1, 0, 0),
            Vec4<T>(s, 0, c, 0),
            Vec4<T>(0, 0, 0, 1)
        );
    }

    static Mat4 rotationZ(T radians) {
        T c = cos(radians), s = sin(radians);
        return Mat4(
            Vec4<T>(c, s, 0, 0),
            Vec4<T>(-s, c, 0, 0),
            Vec4<T>(0, 0, 1, 0),
            Vec4<T>(0, 0, 0, 1)
        );
    }

    static Mat4 perspective(T fovYRadians, T aspect, T zNear, T zFar) {
        T f = T(1) / tan(fovYRadians / T(2));
        return Mat4(
            Vec4<T>(f / aspect, 0, 0, 0),
            Vec4<T>(0, f, 0, 0),
            Vec4<T>(0, 0, (zFar + zNear) / (zNear - zFar), -1),
            Vec4<T>(0, 0, (2 * zFar * zNear) / (zNear - zFar), 0)
        );
    }

    static Mat4 orthographic(T left, T right, T bottom, T top, T zNear, T zFar) {
        return Mat4(
            Vec4<T>(2 / (right - left), 0, 0, 0),
            Vec4<T>(0, 2 / (top - bottom), 0, 0),
            Vec4<T>(0, 0, -2 / (zFar - zNear), 0),
            Vec4<T>(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(zFar + zNear) / (zFar - zNear), 1)
        );
    }

    static Mat4 lookAt(Vec3<T> const& eye, Vec3<T> const& target, Vec3<T> const& up) {
        T fx = target.x - eye.x, fy = target.y - eye.y, fz = target.z - eye.z;
        T flen = sqrt(fx * fx + fy * fy + fz * fz);
        fx /= flen;
        fy /= flen;
        fz /= flen;

        T sx = fy * up.z - fz * up.y;
        T sy = fz * up.x - fx * up.z;
        T sz = fx * up.y - fy * up.x;
        T slen = sqrt(sx * sx + sy * sy + sz * sz);
        sx /= slen;
        sy /= slen;
        sz /= slen;

        T ux = sy * fz - sz * fy;
        T uy = sz * fx - sx * fz;
        T uz = sx * fy - sy * fx;

        return Mat4(
            Vec4<T>(sx, ux, -fx, 0),
            Vec4<T>(sy, uy, -fy, 0),
            Vec4<T>(sz, uz, -fz, 0),
            Vec4<T>(
                -(sx * eye.x + sy * eye.y + sz * eye.z),
                -(ux * eye.x + uy * eye.y + uz * eye.z),
                fx * eye.x + fy * eye.y + fz * eye.z,
                1
            )
        );
    }

    Vec4<T>& operator[](usize i) { return cols[i]; }

    Vec4<T> const& operator[](usize i) const { return cols[i]; }

    friend Vec4<T> operator*(Mat4 const& m, Vec4<T> const& v) {
        return Vec4<T>(
            m.col0.x * v.x + m.col1.x * v.y + m.col2.x * v.z + m.col3.x * v.w,
            m.col0.y * v.x + m.col1.y * v.y + m.col2.y * v.z + m.col3.y * v.w,
            m.col0.z * v.x + m.col1.z * v.y + m.col2.z * v.z + m.col3.z * v.w,
            m.col0.w * v.x + m.col1.w * v.y + m.col2.w * v.z + m.col3.w * v.w
        );
    }

    friend Mat4 operator*(Mat4 const& a, Mat4 const& b) {
        return Mat4(a * b.col0, a * b.col1, a * b.col2, a * b.col3);
    }

    Mat4& operator*=(Mat4 const& o) {
        *this = *this * o;
        return *this;
    }

    friend Mat4 operator+(Mat4 const& a, Mat4 const& b) {
        Mat4 r;
        for (usize i = 0; i < 16; i++)
            r._els[i] = a._els[i] + b._els[i];
        return r;
    }

    friend Mat4 operator-(Mat4 const& a, Mat4 const& b) {
        Mat4 r;
        for (usize i = 0; i < 16; i++)
            r._els[i] = a._els[i] - b._els[i];
        return r;
    }

    friend Mat4 operator*(Mat4 const& a, T s) {
        Mat4 r;
        for (usize i = 0; i < 16; i++)
            r._els[i] = a._els[i] * s;
        return r;
    }

    friend Mat4 operator*(T s, Mat4 const& a) { return a * s; }

    bool operator==(Mat4 const& o) const {
        for (usize i = 0; i < 16; i++)
            if (_els[i] != o._els[i])
                return false;
        return true;
    }

    bool operator!=(Mat4 const& o) const { return !(*this == o); }

    Mat4 transposed() const {
        return Mat4(
            Vec4<T>(col0.x, col1.x, col2.x, col3.x),
            Vec4<T>(col0.y, col1.y, col2.y, col3.y),
            Vec4<T>(col0.z, col1.z, col2.z, col3.z),
            Vec4<T>(col0.w, col1.w, col2.w, col3.w)
        );
    }

    T determinant() const {
        T a00 = col0.x, a01 = col1.x, a02 = col2.x, a03 = col3.x;
        T a10 = col0.y, a11 = col1.y, a12 = col2.y, a13 = col3.y;
        T a20 = col0.z, a21 = col1.z, a22 = col2.z, a23 = col3.z;
        T a30 = col0.w, a31 = col1.w, a32 = col2.w, a33 = col3.w;

        T s0 = a00 * a11 - a10 * a01, s1 = a00 * a12 - a10 * a02, s2 = a00 * a13 - a10 * a03;
        T s3 = a01 * a12 - a11 * a02, s4 = a01 * a13 - a11 * a03, s5 = a02 * a13 - a12 * a03;
        T c5 = a22 * a33 - a32 * a23, c4 = a21 * a33 - a31 * a23, c3 = a21 * a32 - a31 * a22;
        T c2 = a20 * a33 - a30 * a23, c1 = a20 * a32 - a30 * a22, c0 = a20 * a31 - a30 * a21;

        return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    }

    Mat4 inverse() const {
        T a00 = col0.x, a01 = col1.x, a02 = col2.x, a03 = col3.x;
        T a10 = col0.y, a11 = col1.y, a12 = col2.y, a13 = col3.y;
        T a20 = col0.z, a21 = col1.z, a22 = col2.z, a23 = col3.z;
        T a30 = col0.w, a31 = col1.w, a32 = col2.w, a33 = col3.w;

        T s0 = a00 * a11 - a10 * a01, s1 = a00 * a12 - a10 * a02, s2 = a00 * a13 - a10 * a03;
        T s3 = a01 * a12 - a11 * a02, s4 = a01 * a13 - a11 * a03, s5 = a02 * a13 - a12 * a03;
        T c5 = a22 * a33 - a32 * a23, c4 = a21 * a33 - a31 * a23, c3 = a21 * a32 - a31 * a22;
        T c2 = a20 * a33 - a30 * a23, c1 = a20 * a32 - a30 * a22, c0 = a20 * a31 - a30 * a21;

        T det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
        T id = T(1) / det;

        T b00 = (a11 * c5 - a12 * c4 + a13 * c3) * id;
        T b01 = (-a01 * c5 + a02 * c4 - a03 * c3) * id;
        T b02 = (a31 * s5 - a32 * s4 + a33 * s3) * id;
        T b03 = (-a21 * s5 + a22 * s4 - a23 * s3) * id;
        T b10 = (-a10 * c5 + a12 * c2 - a13 * c1) * id;
        T b11 = (a00 * c5 - a02 * c2 + a03 * c1) * id;
        T b12 = (-a30 * s5 + a32 * s2 - a33 * s1) * id;
        T b13 = (a20 * s5 - a22 * s2 + a23 * s1) * id;
        T b20 = (a10 * c4 - a11 * c2 + a13 * c0) * id;
        T b21 = (-a00 * c4 + a01 * c2 - a03 * c0) * id;
        T b22 = (a30 * s4 - a31 * s2 + a33 * s0) * id;
        T b23 = (-a20 * s4 + a21 * s2 - a23 * s0) * id;
        T b30 = (-a10 * c3 + a11 * c1 - a12 * c0) * id;
        T b31 = (a00 * c3 - a01 * c1 + a02 * c0) * id;
        T b32 = (-a30 * s3 + a31 * s1 - a32 * s0) * id;
        T b33 = (a20 * s3 - a21 * s1 + a22 * s0) * id;

        return Mat4(
            Vec4<T>(b00, b10, b20, b30),
            Vec4<T>(b01, b11, b21, b31),
            Vec4<T>(b02, b12, b22, b32),
            Vec4<T>(b03, b13, b23, b33)
        );
    }

    void repr(Io::Emit& e) const {
        e("(mat4 ");
        for (auto& col : cols) {
            e("{}", col);
        }
        e(")");
    }
};

export using Mat4i = Mat4<i64>;
export using Mat4f = Mat4<f64>;

} // namespace Karm::Math
