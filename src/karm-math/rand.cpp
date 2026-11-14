export module Karm.Math:rand;

import :rect;
import :vec;

namespace Karm::Math {

// PCG32 (XSH-RR 64/32), see https://www.pcg-random.org
export struct Rand {
    u64 _state{};
    u64 _inc{};

    Rand() : Rand(0) {}

    Rand(u64 seed, u64 seq = 0xda3e39cb94b95bdbULL)
        : _inc((seq << 1) | 1) {
        nextU32();
        _state += seed;
        nextU32();
    }

    // MARK: Integers ----------------------------------------------------------

    u32 nextU32() {
        u64 oldstate = _state;

        _state = oldstate * 6364136223846793005ULL + (_inc | 1);
        u32 xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
        u32 rot = oldstate >> 59u;

        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

    u64 nextU64() {
        u64 lo = nextU32();
        u64 hi = nextU32();
        return lo | hi << 32;
    }

    u16 nextU16() {
        return nextU32();
    }

    u8 nextU8() {
        return nextU32();
    }

    u64 nextU64(u64 max) {
        if (max == 0)
            return 0;

        u64 threshold = (0 - max) % max;
        while (true) {
            u64 r = nextU64();
            if (r >= threshold)
                return r % max;
        }
    }

    isize nextInt() {
        return static_cast<isize>(static_cast<usize>(nextU64()) >> 1);
    }

    isize nextInt(isize min, isize max) {
        if (max <= min)
            return min;

        u64 range = static_cast<u64>(max) - static_cast<u64>(min);
        return static_cast<isize>(static_cast<u64>(min) + nextU64(range));
    }

    isize nextInt(isize max) {
        return nextInt(0, max);
    }

    bool nextBool() {
        return nextU32() & 1;
    }

    // MARK: Floating point ----------------------------------------------------

    f64 nextDouble() {
        return (nextU64() >> 11) * 0x1.0p-53;
    }

    f64 nextDouble(f64 min, f64 max) {
        return min + nextDouble() * (max - min);
    }

    f64 nextDouble(f64 max) {
        return nextDouble() * max;
    }

    f32 nextFloat() {
        return (nextU32() >> 8) * 0x1.0p-24f;
    }

    f32 nextFloat(f32 min, f32 max) {
        return min + nextFloat() * (max - min);
    }

    f32 nextFloat(f32 max) {
        return nextFloat() * max;
    }

    // MARK: Geometry ----------------------------------------------------------

    template <typename T>
    T _nextIn(T min, T max) {
        if constexpr (T(1) / T(2) == T(0))
            return static_cast<T>(nextInt(static_cast<isize>(min), static_cast<isize>(max)));
        else
            return static_cast<T>(nextDouble(min, max));
    }

    template <typename T>
    Vec2<T> nextVec2(Rect<T> bound) {
        return Vec2<T>(
            _nextIn<T>(bound.start(), bound.end()),
            _nextIn<T>(bound.top(), bound.bottom())
        );
    }

    template <typename T>
    Vec2<T> nextVec2(Vec2<T> max) {
        return Vec2<T>(
            _nextIn<T>(T{}, max.x),
            _nextIn<T>(T{}, max.y)
        );
    }

    // MARK: Collections -------------------------------------------------------

    auto pick(Sliceable auto const& vals) {
        return vals[nextU64(vals.len())];
    }

    void shuffle(MutSliceable auto& vals) {
        for (usize i = vals.len(); i > 1; --i) {
            usize j = nextU64(i);
            std::swap(vals[i - 1], vals[j]);
        }
    }
};

} // namespace Karm::Math