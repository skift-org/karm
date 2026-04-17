export module Karm.Core:base.hash;

import :meta.traits;
import :base.base;

namespace Karm {

export struct Hasher {
    virtual ~Hasher() = default;
    virtual void add(u8 const* buf, usize len) = 0;
    virtual u64 finish() = 0;
};

// https://www.ietf.org/archive/id/draft-eastlake-fnv-21.html
export struct FnvHasher : Hasher {
    u64 hash = 0xcbf29ce484222325;

    void add(u8 const* buf, usize len) override {
        for (usize i = 0; i < len; i++) {
            hash ^= buf[i];
            hash *= 0x100000001b3;
        }
    }

    u64 finish() override {
        return hash;
    }
};

using DefaultHasher = FnvHasher;

template <typename T>
struct Hash;

export template <typename T>
constexpr void hash(Meta::Derive<Hasher> auto& hasher, T const& t) {
    if constexpr (
        requires(Hasher& hasher, T const t) {
            { t.hash(hasher) };
        }
    ) {
        t.hash(hasher);
    } else {
        Hash<T>::hash(hasher, t);
    }
}

export template <typename T>
constexpr u64 hash(T const& t) {
    DefaultHasher hasher;
    hash(hasher, t);
    return hasher.finish();
}

export template <Meta::Boolean T>
struct Hash<T> {
    static constexpr void hash(Meta::Derive<Hasher> auto& hasher, T const& v) {
        hasher.add(reinterpret_cast<u8 const*>(&v), sizeof(v));
    }
};

export template <Meta::Integer T>
struct Hash<T> {
    static constexpr void hash(Meta::Derive<Hasher> auto& hasher, T const& v) {
        hasher.add(reinterpret_cast<u8 const*>(&v), sizeof(v));
    }
};

export template <Meta::Float T>
struct Hash<T> {
    static constexpr void hash(Meta::Derive<Hasher> auto& hasher, T const& v) {
        hasher.add(reinterpret_cast<u8 const*>(&v), sizeof(v));
    }
};

export template <Meta::Enum T>
struct Hash<T> {
    static constexpr void hash(Meta::Derive<Hasher> auto& hasher, T const& v) {
        hasher.add(reinterpret_cast<u8 const*>(&v), sizeof(v));
    }
};

export template <>
struct Hash<None> {
    static constexpr void hash(Meta::Derive<Hasher> auto& hasher, None const&) {
        Karm::hash(hasher, 0x0);
    }
};

} // namespace Karm
