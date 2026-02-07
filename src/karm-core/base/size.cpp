export module Karm.Core:base.size;

import :base.base;
import :base.distinct;

namespace Karm {

export struct DataSize : Distinct<u64, struct _DataSizeTag> {
    using Distinct::Distinct;

    static constexpr DataSize fromKiB(u64 v) { return DataSize{v * 1024}; }

    static constexpr DataSize fromMiB(u64 v) { return fromKiB(v * 1024); }

    static constexpr DataSize fromGiB(u64 v) { return fromMiB(v * 1024); }

    static constexpr DataSize fromTiB(u64 v) { return fromGiB(v * 1024); }

    constexpr u64 toB() const { return _value; }

    constexpr u64 toKiB() const { return _value / 1024; }

    constexpr u64 toMiB() const { return toKiB() / 1024; }

    constexpr u64 toGiB() const { return toMiB() / 1024; }

    constexpr u64 toTiB() const { return toGiB() / 1024; }

    constexpr operator u64() const { return _value; }
};

} // namespace Karm

export constexpr Karm::DataSize operator""_B(unsigned long long val) {
    return Karm::DataSize{val};
}

export constexpr Karm::DataSize operator""_KiB(unsigned long long val) {
    return Karm::DataSize::fromKiB(val);
}

export constexpr Karm::DataSize operator""_MiB(unsigned long long val) {
    return Karm::DataSize::fromMiB(val);
}

export constexpr Karm::DataSize operator""_GiB(unsigned long long val) {
    return Karm::DataSize::fromGiB(val);
}

export constexpr Karm::DataSize operator""_TiB(unsigned long long val) {
    return Karm::DataSize::fromGiB(val);
}
