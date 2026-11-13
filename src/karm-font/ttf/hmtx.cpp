module;

#include <karm/macros>

export module Karm.Font.Ttf:hmtx;

import :hhea;

namespace Karm::Font::Ttf {

export struct Hmtx : Io::BChunk {
    static constexpr Str SIG = "hmtx";
    static constexpr isize LONG_RECORD_SIZE = 4;
    static constexpr isize SHORT_RECORD_SIZE = 2;

    struct Metrics {
        isize advanceWidth;
        isize lsb;
    };

    Res<Metrics> metrics(isize glyphId, Hhea const& hhea) const {
        if (glyphId < hhea.numberOfHMetrics()) {
            auto s = try$(begin(glyphId * 4));
            isize adv = try$(s.nextU16be());
            isize lsb = try$(s.nextI16be());
            return Ok(Metrics{adv, lsb});
        }

        if (hhea.numberOfHMetrics() == 0)
            return Error::invalidData("hmtx: no long horizontal metrics");

        usize advOffset = (hhea.numberOfHMetrics() - 1) * LONG_RECORD_SIZE;
        usize lsbOffset = hhea.numberOfHMetrics() * LONG_RECORD_SIZE +
                          (glyphId - hhea.numberOfHMetrics()) * SHORT_RECORD_SIZE;

        auto advScan = try$(begin(advOffset));
        isize adv = try$(advScan.nextU16be());
        auto lsbScan = try$(begin(lsbOffset));
        isize lsb = try$(lsbScan.nextI16be());

        return Ok(Metrics{adv, lsb});
    }
};

} // namespace Karm::Font::Ttf
