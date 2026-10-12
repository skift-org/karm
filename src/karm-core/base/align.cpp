export module Karm.Core:base.align;

import :base.opt;

namespace Karm {

// Align the nearest _lower_ aligned address
// ex: 8 with align = 8 -> 8
// ex: 9 with align = 8 -> 8
// ex: 7 with align = 8 -> 0
// ex: 25 with align = 24 -> 24
export constexpr usize alignDown(usize addr, usize align) {
    return addr - addr % align;
}

// Align the nearest _upper_ aligned address
// ex: 8 with align = 8 -> 8
// ex: 9 with align = 8 -> 16
// ex: 7 with align = 8 -> 8
// ex: 25 with align = 24 -> 48
export constexpr usize alignUp(usize addr, usize align) {
    return alignDown(addr + align - 1, align);
}

// Divide rounding toward the upper integer
// ex: 16 / 8 -> 2
// ex: 17 / 8 -> 3
// ex: 0 / 8 -> 0
export constexpr usize ceilDiv(usize n, usize d) {
    return (n + d - 1) / d;
}

export constexpr bool aligned(usize addr, usize align) {
    return alignDown(addr, align) == addr;
}

export constexpr Opt<bool> ensureAlign(usize addr, usize align) {
    if (not aligned(addr, align))
        return NONE;
    return Some(true);
}

} // namespace Karm
