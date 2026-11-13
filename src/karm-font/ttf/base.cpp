export module Karm.Font.Ttf:base;

import Karm.Core;

namespace Karm::Font::Ttf {

export struct Fixed {
    u32be value;

    f64 asF64() const {
        return f64(value) / 65536.0;
    }
};

export struct Version {
    u16be major;
    u16be minor;
};

// https://learn.microsoft.com/en-us/typography/opentype/spec/otff#calculating-checksums
export u32 checksum(Bytes b) {
    Io::BScan s{b};
    u32 sum = 0;
    while (not s.ended()) {
        u32 word = 0;
        for (usize j = 0; j < 4; j++)
            word = (word << 8) | s.nextU8be().unwrapOr(0);
        sum += word;
    }
    return sum;
}

} // namespace Karm::Font::Ttf
