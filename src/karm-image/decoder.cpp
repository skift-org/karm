export module Karm.Image:decoder;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;
import Karm.Ref;

namespace Karm::Image {

export struct Metadata {
    Math::Vec2i size;
    Gfx::ColorSpace colorSpace;
    u8 bitDepth;
    bool invertedColors;
};

export struct Decoder {
    static Res<Box<Decoder>> createFrom(Io::Reader& reader, Ref::Uti format);
    static Res<Box<Decoder>> createFrom(Bytes buf, Ref::Uti format);

    virtual ~Decoder() = default;
    virtual Metadata metadata() = 0;
    virtual Res<> decode(Gfx::MutPixels pixels) = 0;
};

} // namespace Karm::Image
