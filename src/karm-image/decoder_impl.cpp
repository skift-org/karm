module Karm.Image;

import :jpeg.decoder;

namespace Karm::Image {

Res<Box<Decoder>> Decoder::createFrom(Bytes buf, Ref::Uti format) {
    if (format == Ref::Uti::PUBLIC_JPEG) {
        return Jpeg::createDecoder(buf);
    } else {
        // FIXME: Better error
        return Error::notImplemented("no decoder for this format");
    }
}

Res<Box<Decoder>> Decoder::createFrom(Io::Reader& reader, Ref::Uti format) {
    (void)reader;
    (void)format;
    panic("not implemented");
}

} // namespace Karm::Image
