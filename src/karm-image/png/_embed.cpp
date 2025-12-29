export module Karm.Image:png._embed;

import Karm.Core;

import :decoder;

namespace Karm::Image::Png::_Embed {

Res<Box<Decoder>> createPngDecoder(Bytes bytes);

} // namespace Karm::Image::Png::_Embed
