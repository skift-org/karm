export module Karm.Image:jpeg.decoder;

import Karm.Core;
import Karm.Gfx;
import Karm.Logger;

import :jpeg._embed;

namespace Karm::Image::Jpeg {

export Res<Box<Decoder>> createDecoder(Bytes bytes) {
    return _Embed::createJpegDecoder(bytes);
}

} // namespace Karm::Image::Jpeg
