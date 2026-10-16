#include <avif/avif.h>
#include <karm/macros>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;

namespace Karm::Image::Libavif {

static Error _error(avifResult result) {
    return Error::invalidData(String{Str::fromNullterminated(avifResultToString(result))});
}

static bool _sniff(Bytes bytes) {
    avifROData data{bytes.buf(), bytes.len()};
    return avifPeekCompatibleFileType(&data);
}

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    avifDecoder* decoder = avifDecoderCreate();
    if (not decoder)
        return Error::outOfMemory("could not create avif decoder");
    Defer _ = [&] {
        avifDecoderDestroy(decoder);
    };

    if (auto result = avifDecoderSetIOMemory(decoder, bytes.buf(), bytes.len()); result != AVIF_RESULT_OK)
        return _error(result);
    if (auto result = avifDecoderParse(decoder); result != AVIF_RESULT_OK)
        return _error(result);
    if (auto result = avifDecoderNextImage(decoder); result != AVIF_RESULT_OK)
        return _error(result);

    auto img = Gfx::Image::alloc({(isize)decoder->image->width, (isize)decoder->image->height}, Gfx::RGBA8888);

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, decoder->image);
    rgb.format = AVIF_RGB_FORMAT_RGBA;
    rgb.depth = 8;
    rgb.pixels = img->mutBytes().buf();
    rgb.rowBytes = img->stride();

    if (auto result = avifImageYUVToRGB(decoder->image, &rgb); result != AVIF_RESULT_OK)
        return _error(result);

    return Ok(img);
}

static Codec _codec{
    "libavif",
    Ref::Uti::PUBLIC_AVIF,
    Codec::MAGIC,
    Codec::SYSTEM,
    _sniff,
    _decode,
};

} // namespace Karm::Image::Libavif
