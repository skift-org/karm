export module Karm.Core:base.bloom;

import :base.array;
import :base.vec;
import :base.bits;
import :base.size;
import :base.range;
import :math.funcs;

namespace Karm {

export template <typename T>
struct Bloom {
    static constexpr u64 SEED1 = 0x9e3779b97f4a7c15;
    static constexpr u64 SEED2 = 0x517cc1b727220a95;

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

    Pair<u64, u64> _hashPair(Meta::Equatable<T> auto const& value) const {
        DefaultHasher hasher1;
        Karm::hash(hasher1, SEED1);
        Karm::hash(hasher1, value);

        DefaultHasher hasher2;
        Karm::hash(hasher2, SEED2);
        Karm::hash(hasher2, value);

        return {hasher1.finish(), hasher2.finish()};
    }

    void add(Meta::Equatable<T> auto const& value) {
        MutBits bits{_buf};
        auto [h1, h2] = _hashPair(value);
        usize m = bits.len();

        // https://en.wikipedia.org/wiki/Double_hashing
        for (usize i : urange::zeroTo(_k)) {
            usize index = (h1 + i * h2) % m;
            bits.set(index, true);
        }
    };

    bool maybeContains(Meta::Equatable<T> auto const& value) const {
        Bits bits{_buf};
        auto [h1, h2] = _hashPair(value);
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


} // namespace Karm
