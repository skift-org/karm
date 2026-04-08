export module Karm.Core:icu.segment;

import :base.string;

namespace Karm::Icu {

// Implementation of the unicode text segmentation algorithm
// https://unicode.org/reports/tr29/
export usize countGrapheneClusters(Str s) {
    // FIXME: This is not how you do that 🤪
    usize n = 0;
    for (auto _ : iterRunes(s))
        n++;
    return n;
}

} // namespace Karm::Icu
