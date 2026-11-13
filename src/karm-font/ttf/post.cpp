export module Karm.Font.Ttf:post;

import Karm.Core;

import :base;

namespace Karm::Font::Ttf {

export struct Post : Io::BChunk {
    static constexpr Str SIG = "post";

    using ItalicAngle = Io::BField<Fixed, 4>;
    using IsFixedPitch = Io::BField<u32be, 12>;

    bool isFixedPitch() const {
        return get<IsFixedPitch>().unwrapOr(0);
    }

    f64 italicAngle() const {
        return get<ItalicAngle>().unwrapOr(Fixed{0}).asF64();
    }
};

} // namespace Karm::Font::Ttf
