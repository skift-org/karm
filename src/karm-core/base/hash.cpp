export module Karm.Core:base.hash;

import :meta.traits;
import :base.base;

namespace Karm {

// https://www.ietf.org/archive/id/draft-eastlake-fnv-21.html
export constexpr u64 fnv64(u8 const* buf, usize len) {
    u64 hash = 0xcbf29ce484222325;
    for (usize i = 0; i < len; i++) {
        hash ^= buf[i];
        hash *= 0x100000001b3;
    }
    return hash;
}

template <typename T>
struct Hash;

export template <typename T>
constexpr u64 hash(T const& t) {
    if constexpr (
        requires(T const t) {
            { t.hash() } -> Meta::Same<u64>;
        }
    ) {
        return t.hash();
    } else {
        return Hash<T>::hash(t);
    }
}

export template <Meta::Boolean T>
struct Hash<T> {
    static constexpr u64 hash(T const& v) {
        return fnv64(reinterpret_cast<u8 const*>(&v), sizeof(v));
    }
};

export template <Meta::Integer T>
struct Hash<T> {
    static constexpr u64 hash(T const& v) {
        return fnv64(reinterpret_cast<u8 const*>(&v), sizeof(v));
    }
};

export template <Meta::Float T>
struct Hash<T> {
    static constexpr u64 hash(T const& v) {
        return fnv64(reinterpret_cast<u8 const*>(&v), sizeof(v));
    }
};

export template <Meta::Enum T>
struct Hash<T> {
    static constexpr u64 hash(T const& v) {
        return fnv64(reinterpret_cast<u8 const*>(&v), sizeof(v));
    }
};

export template <>
struct Hash<None> {
    static constexpr u64 hash(None const&) {
        return Karm::hash(0x0);
    }
};

} // namespace Karm
