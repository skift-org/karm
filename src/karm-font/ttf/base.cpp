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

} // namespace Karm::Font::Ttf
