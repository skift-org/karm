module Karm.Image;

import :jpeg.decoder;
import :png.decoder;
import :bmp.decoder;

namespace Karm::Image {

Res<Box<Decoder>> Decoder::createFrom(Bytes buf, Ref::Uti format) {
    if (format == Ref::Uti::PUBLIC_JPEG) {
        return Jpeg::createDecoder(buf);
    } else if (format == Ref::Uti::PUBLIC_PNG) {
        return Png::createPngDecoder(buf);
    } else if (format == Ref::Uti::PUBLIC_BMP) {
        return Bmp::createDecoder(buf);
    } else {
        // FIXME: Better error
        return Error::notImplemented("no decoder for this format");
    }
}

Res<Box<Decoder>> Decoder::createFrom(Io::Reader& reader, Ref::Uti format) {
    (void)reader;
    (void)format;
    return Error::notImplemented();
}

} // namespace Karm::Image
