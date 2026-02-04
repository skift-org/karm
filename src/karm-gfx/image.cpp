export module Karm.Gfx:image;

import Karm.Icc;
import Karm.Math;

import :buffer;

namespace Karm::Gfx {

export struct Image {
    struct Metadata {
        Icc::ColorProfile colorProfile;
        Math::Vec2i size;
        u8 bitDepth;
        bool isInverted;
    };

    virtual ~Image() = default;

    virtual Bytes bytes() = 0;

    virtual Metadata const& metadata() = 0;

    virtual Res<Rc<Surface>> decode(Opt<Fmt> fmt = NONE) = 0;
};

} // namespace Karm::Gfx
