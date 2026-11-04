export module Karm.Font.Ttf:os2;

import Karm.Core;

import :base;

namespace Karm::Font::Ttf {

export struct Os2 : Io::BChunk {
    static constexpr Str SIG = "OS/2";

    using Version = Io::BField<u16be, 0>;
    using WeightClass = Io::BField<u16be, 4>;
    using WidthClass = Io::BField<u16be, 6>;
    using XHeight = Io::BField<i16be, 86>;
    using CapHeight = Io::BField<i16be, 88>;

    u16 version() const {
        return get<Version>();
    }

    u16 weightClass() const {
        return get<WeightClass>();
    }

    u16 widthClass() const {
        return get<WidthClass>();
    }

    // sxHeight and sCapHeight only exist from version 2 on.
    bool _hasHeights() const {
        return version() >= 2 and bytes().len() >= CapHeight::offset + sizeof(i16be);
    }

    Opt<i16> xHeight() const {
        if (not _hasHeights())
            return NONE;
        return Some<i16>(get<XHeight>());
    }

    Opt<i16> capHeight() const {
        if (not _hasHeights())
            return NONE;
        return Some<i16>(get<CapHeight>());
    }
};

} // namespace Karm::Font::Ttf
