export module Karm.Icu:segment;

import Karm.Core;

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
