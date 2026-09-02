export module Karm.Core:base.bloom;

import :base.array;
import :base.vec;
import :base.bits;
import :base.size;
import :base.range;
import :math.funcs;

using namespace Karm::Literals;

namespace Karm {

template <typename V>
Pair<u64, u64> _hashItemPair(V const& value) {
    DefaultHasher hasher;
    Karm::hash(hasher, value);

    u64 h = hasher.finish();

    // NOTE: The hash quality of DefaultHasher might not be
    //       the same on the high bits and low bits so
    //       we scramble using fmix64 from MurmurHash3.
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;

    return {h & 0xffffffff, h >> 32};
}

export template <typename T>
struct Bloom {
    Vec<u8> _buf;
    usize _k;

#ifndef __ck_freestanding__
    static Bloom optimal(usize expectedItems, f64 targetErrorRate) {
        f64 m = -(static_cast<f64>(expectedItems) * Math::log(targetErrorRate)) / (Math::LOG2 * Math::LOG2);
        usize k = Math::roundi(m / static_cast<f64>(expectedItems) * Math::LOG2);
        if (k < 1)
            k = 1;
        usize bytes = Math::roundi(Math::ceil(m / 8.0));
        return Bloom(bytes, k);
    }
#endif

    Bloom(usize size = 16_KiB, usize k = 2) : _k(k) {
        _buf.resize(size);
    }

    void add(Meta::Equatable<T> auto const& value) {
        MutBits bits{_buf};
        auto [h1, h2] = _hashItemPair(value);
        usize m = bits.len();

        // https://en.wikipedia.org/wiki/Double_hashing
        for (usize i : urange::zeroTo(_k)) {
            usize index = (h1 + i * h2) % m;
            bits.set(index, true);
        }
    };

    bool maybeContains(Meta::Equatable<T> auto const& value) const {
        Bits bits{_buf};
        auto [h1, h2] = _hashItemPair(value);
        usize m = bits.len();
        for (usize i : urange::zeroTo(_k)) {
            // https://en.wikipedia.org/wiki/Double_hashing
            usize index = (h1 + i * h2) % m;
            if (not bits.get(index))
                return false;
        }
        return true;
    }

    void clear() {
        zeroFill(mutBytes(_buf));
    }
};

/// Bloom filter that supports removal, but is less space efficient than the normal one.
export template <typename T, usize N = 4096>
struct CountingBloom {
    static_assert((N & (N - 1)) == 0, "CountingBloom size must be a power of two");

    static constexpr usize MASK = N - 1;
    static constexpr u8 SATURATED = ~u8{0};

    Array<u8, N> _slots = {};

    void _increment(usize slot) {
        if (_slots[slot] != SATURATED)
            ++_slots[slot];
    }

    void _decrement(usize slot) {
        // NOTE: Once a slot is saturated, we cannot remove from it anymore.
        if (_slots[slot] != SATURATED)
            --_slots[slot];
    }

    void add(Meta::Equatable<T> auto const& value) {
        auto [h1, h2] = _hashItemPair(value);
        _increment(h1 & MASK);
        _increment(h2 & MASK);
    }

    void remove(Meta::Equatable<T> auto const& value) {
        auto [h1, h2] = _hashItemPair(value);
        _decrement(h1 & MASK);
        _decrement(h2 & MASK);
    }

    bool maybeContains(Meta::Equatable<T> auto const& value) const {
        auto [h1, h2] = _hashItemPair(value);
        return _slots[h1 & MASK] and _slots[h2 & MASK];
    }

    void clear() {
        for (usize i = 0; i < N; i++)
            _slots[i] = 0;
    }
};

} // namespace Karm
