module;

#include <karm/macros>

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
        return get<Version>().unwrapOr(0);
    }

    // NOTE: Falls back to FW_NORMAL if the table is truncated.
    u16 weightClass() const {
        return get<WeightClass>().unwrapOr(400);
    }

    // NOTE: Falls back to FWIDTH_NORMAL if the table is truncated.
    u16 widthClass() const {
        return get<WidthClass>().unwrapOr(5);
    }

    // sxHeight and sCapHeight only exist from version 2 on.
    bool _hasHeights() const {
        return version() >= 2 and bytes().len() >= CapHeight::offset + sizeof(i16be);
    }

    Opt<i16> xHeight() const {
        if (not _hasHeights())
            return NONE;
        return Some<i16>(try$(get<XHeight>().ok()));
    }

    Opt<i16> capHeight() const {
        if (not _hasHeights())
            return NONE;
        return Some<i16>(try$(get<CapHeight>().ok()));
    }
};

} // namespace Karm::Font::Ttf
