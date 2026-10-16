#include <karm/macros>
#include <webp/decode.h>
#include <webp/encode.h>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;

using namespace Karm::Literals;

namespace Karm::Image::Libwebp {

static bool _sniff(Bytes bytes) {
    return bytes.len() >= 12 and
           sub(bytes, 0, 4) == "RIFF"_bytes and
           sub(bytes, 8, 12) == "WEBP"_bytes;
}

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    int width = 0, height = 0;
    if (not WebPGetInfo(bytes.buf(), bytes.len(), &width, &height))
        return Error::invalidData("invalid webp header");

    auto img = Gfx::Image::alloc({width, height}, Gfx::RGBA8888);
    auto out = img->mutBytes();
    if (not WebPDecodeRGBAInto(bytes.buf(), bytes.len(), out.buf(), out.len(), img->stride()))
        return Error::invalidData("could not decode webp image");

    return Ok(img);
}

static Res<> _encode(Gfx::Pixels pixels, Io::BEmit& e, Saver const& props) {
    Opt<Rc<Gfx::Image>> converted;
    if (pixels.fmt() == Gfx::GREYSCALE8) {
        auto img = Gfx::Image::alloc(pixels.size(), Gfx::RGBA8888);
        Gfx::blitUnsafe(img->mutPixels(), pixels);
        pixels = img->pixels();
        converted = Some(img);
    }

    bool bgra = pixels.fmt() == Gfx::BGRA8888;
    u8 const* src = pixels.bytes().buf();
    uint8_t* buf = nullptr;
    usize size = 0;

    if (props.quality >= Saver::MAX_QUALITY) {
        size = bgra
                   ? WebPEncodeLosslessBGRA(src, pixels.width(), pixels.height(), pixels.stride(), &buf)
                   : WebPEncodeLosslessRGBA(src, pixels.width(), pixels.height(), pixels.stride(), &buf);
    } else {
        f32 quality = clamp(props.quality, Saver::MIN_QUALITY, Saver::MAX_QUALITY) * 100;
        size = bgra
                   ? WebPEncodeBGRA(src, pixels.width(), pixels.height(), pixels.stride(), quality, &buf)
                   : WebPEncodeRGBA(src, pixels.width(), pixels.height(), pixels.stride(), quality, &buf);
    }

    Defer _ = [&] {
        WebPFree(buf);
    };

    if (not size)
        return Error::invalidData("could not encode webp image");

    return e.writeBytes({buf, size});
}

static Codec _codec{
    "libwebp",
    Ref::Uti::PUBLIC_WEBP,
    Codec::MAGIC,
    Codec::SYSTEM,
    _sniff,
    _decode,
    _encode,
};

} // namespace Karm::Image::Libwebp
