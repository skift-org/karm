export module Karm.Core:base.size;

import :base.base;
import :base.distinct;

namespace Karm {

export struct DataSize : Distinct<usize, struct _DataSizeTag> {
    using Distinct::Distinct;

    static constexpr DataSize fromKiB(usize v) { return DataSize{v * 1024}; }

    static constexpr DataSize fromMiB(usize v) { return fromKiB(v * 1024); }

    static constexpr DataSize fromGiB(usize v) { return fromMiB(v * 1024); }

    static constexpr DataSize fromTiB(usize v) { return fromGiB(v * 1024); }

    constexpr usize toB() const { return _value; }

    constexpr usize toKiB() const { return _value / 1024; }

    constexpr usize toMiB() const { return toKiB() / 1024; }

    constexpr usize toGiB() const { return toMiB() / 1024; }

    constexpr usize toTiB() const { return toGiB() / 1024; }

    operator usize () const { return _value; }
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
