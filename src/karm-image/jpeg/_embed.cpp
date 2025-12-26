export module Karm.Image:jpeg._embed;

import Karm.Core;

import :decoder;

namespace Karm::Image::Jpeg::_Embed {

Res<Box<Decoder>> createJpegDecoder(Bytes bytes);

} // namespace Karm::Image::Jpeg::_Embed
