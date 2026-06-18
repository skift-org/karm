export module Karm.Ml:tensor;

import Karm.Core;

namespace Karm::Ml {

export struct Tensor {
    using Shape = Tuple<usize, usize, usize, usize>;
    Shape shape;
    Union<f32, MutSlice<f32>, Vec<f32>> _store;

    static Tensor alloc(Shape shape) {
        Tensor res{0};
        res.shape = shape;
        res._store = Vec{Buf<f32>::init(shape.v0 * shape.v1 * shape.v2 * shape.v3)};
        return res;
    }

    static Tensor alloc(usize m = 1, usize n = 1, usize o = 1, usize p = 1) {
        return alloc(Shape{m, n, o, p});
    }

    Tensor(f32 scalar) : shape({1, 1, 1, 1}), _store(scalar) {}

    Tensor(std::initializer_list<f32> t1)
        : shape({t1.size(), 1, 1, 1}),
          _store(Vec<f32>{t1}) {
    }

    Tensor(std::initializer_list<std::initializer_list<f32>> t2)
        : shape({t2.begin()[0].size(), t2.size(), 1, 1}),
          _store(0.f) {
        Vec<f32> vec;
        vec.ensure(shape.v0 * shape.v1);
        for (auto t1 : t2)
            for (auto s : t1)
                vec.pushBack(s);
        _store = std::move(vec);
    }

    Tensor(std::initializer_list<std::initializer_list<std::initializer_list<f32>>> t3)
        : shape({t3.begin()[0].begin()[0].size(), t3.begin()[0].size(), t3.size(), 1}),
          _store(0.f) {
        Vec<f32> vec;
        vec.ensure(shape.v0 * shape.v1 * shape.v2);
        for (auto t2 : t3)
            for (auto t1 : t2)
                for (auto s : t1)
                    vec.pushBack(s);
        _store = std::move(vec);
    }

    Tensor(std::initializer_list<std::initializer_list<std::initializer_list<std::initializer_list<f32>>>> t4)
        : shape({t4.begin()[0].begin()[0].begin()[0].size(), t4.begin()[0].begin()[0].size(), t4.begin()[0].size(), t4.size()}),
          _store(0.f) {
        Vec<f32> vec;
        vec.ensure(shape.v0 * shape.v1 * shape.v2 * shape.v3);
        for (auto t3 : t4)
            for (auto t2 : t3)
                for (auto t1 : t2)
                    for (auto s : t1)
                        vec.pushBack(s);
        _store = std::move(vec);
    }

    MutSlice<f32> elements() {
        return _store.visit(
            [](f32& v) {
                return MutSlice{&v, 1};
            },
            [](auto& v) {
                return mutSub(v);
            }
        );
    }

    Slice<f32> elements() const {
        return _store.visit(
            [](f32 const& v) {
                return Slice{&v, 1};
            },
            [](auto const& v) {
                return sub(v);
            }
        );
    }

    f32& operator[](usize x = 0, usize y = 0, usize z = 0, usize w = 0) {
        auto [m, n, o, p] = shape;
        if (x >= m) [[unlikely]]
            panic("index out of bound");
        if (y >= n) [[unlikely]]
            panic("index out of bound");
        if (z >= o) [[unlikely]]
            panic("index out of bound");
        if (w >= p) [[unlikely]]
            panic("index out of bound");
        return elements()[x + y * m + z * (m * n) + w * (m * n * o)];
    }

    f32 const& operator[](usize x = 0, usize y = 0, usize z = 0, usize w = 0) const {
        auto [m, n, o, p] = shape;
        if (x >= m) [[unlikely]]
            panic("index out of bound");
        if (y >= n) [[unlikely]]
            panic("index out of bound");
        if (z >= o) [[unlikely]]
            panic("index out of bound");
        if (w >= p) [[unlikely]]
            panic("index out of bound");
        return elements()[x + y * m + z * (m * n) + w * (m * n * o)];
    }

    void repr(Io::Emit& e) const {
        auto [m, n, o, p] = shape;
        if (p > 1)
            e("[");
        for (auto w : urange::zeroTo(p)) {
            if (w > 0)
                e(", ");
            if (o > 1)
                e("[");
            for (auto z : urange::zeroTo(o)) {
                if (z > 0)
                    e(", ");
                if (n > 1)
                    e("[");
                for (auto y : urange::zeroTo(n)) {
                    if (y > 0)
                        e(", ");
                    if (m > 1)
                        e("[");
                    for (auto x : urange::zeroTo(m))
                        e(x ? ", {}" : "{}", (*this)[x, y, z, w]);
                    if (m > 1)
                        e("]");
                }
                if (n > 1)
                    e("]");
            }
            if (o > 1)
                e("]");
        }
        if (p > 1)
            e("]");
    }

    bool operator==(Tensor const& other) const {
        return shape == other.shape and elements() == other.elements();
    }
};

export namespace Kernels {

/// Element-wise addition
void add(Tensor& out, Tensor const& a, Tensor const& b) {
    for (auto i : urange::zeroTo(a.elements().len()))
        out.elements()[i] = a.elements()[i] + b.elements()[i];
}

/// Element-wise substraction
void sub(Tensor& out, Tensor const& a, Tensor const& b) {
    for (auto i : urange::zeroTo(a.elements().len()))
        out.elements()[i] = a.elements()[i] - b.elements()[i];
}

/// Element-wise ReLU
/// See: https://en.wikipedia.org/wiki/Rectified_linear_unit
void reLu(Tensor& out, Tensor const& a) {
    for (auto i : urange::zeroTo(a.elements().len()))
        out.elements()[i] = max(0.f, a.elements()[i]);
}

/// Element-Wise heaviside
/// https://en.wikipedia.org/wiki/Heaviside_step_function
void heaviside(Tensor& out, Tensor const& a) {
    for (auto i : urange::zeroTo(a.elements().len())) {
        auto x = a.elements()[i];
        out.elements()[i] = x >= 0 ? 1 : 0;
    }
}

/// Matrix multiplication
/// https://en.wikipedia.org/wiki/Matrix_multiplication
void matMul(Tensor& out, Tensor const& a, Tensor const& b) {
    auto& [m_a, n, _, _] = a.shape;
    auto& [p, m_b, _, _] = b.shape;

    if (m_a != m_b) [[unlikely]]
        panic("inner dimensions must match");

    for (usize i : urange::zeroTo(n)) {
        for (usize j : urange::zeroTo(p)) {
            f64 sum = 0;
            for (usize k : urange::zeroTo(m_a))
                sum += a[k, i] * b[j, k];
            out[j, i] = sum;
        }
    }
}

} // namespace Kernels

} // namespace Karm::Ml
