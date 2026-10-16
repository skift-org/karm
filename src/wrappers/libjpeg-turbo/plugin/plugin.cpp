#include <karm/macros>
#include <turbojpeg.h>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Math;
import Karm.Ref;

namespace Karm::Image::LibjpegTurbo {

// NOTE: This uses the TurboJPEG 2 API since the tj3 one is only available in
//       libjpeg-turbo 3.0+, which most distros don't ship yet.

static Error _error(tjhandle handle) {
    return Error::invalidData(String{Str::fromNullterminated(tjGetErrorStr2(handle))});
}

static bool _sniff(Bytes bytes) {
    return bytes.len() >= 3 and
           bytes[0] == 0xFF and
           bytes[1] == 0xD8 and
           bytes[2] == 0xFF;
}

static Res<Rc<Gfx::Image>> _decodeCmyk(tjhandle handle, Bytes bytes, isize width, isize height) {
    auto cmyk = Vec<u8>::filled(width * height * 4);
    if (tjDecompress2(handle, bytes.buf(), bytes.len(), cmyk.buf(), width, width * 4, height, TJPF_CMYK, 0) < 0)
        return _error(handle);

    // NOTE: Converting CMYK properly requires a color management system, like
    //       browsers we assume Adobe style inverted CMYK, which is what
    //       virtually all CMYK JPEG in the wild are.
    auto img = Gfx::Image::alloc({width, height}, Gfx::RGBA8888);
    auto pixels = img->mutPixels();
    for (isize y = 0; y < height; y++) {
        for (isize x = 0; x < width; x++) {
            u8 const* p = &cmyk[(y * width + x) * 4];
            u8 k = p[3];
            pixels.store({x, y}, Gfx::Color::fromRgb(p[0] * k / 255, p[1] * k / 255, p[2] * k / 255));
        }
    }

    return Ok(img);
}

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    tjhandle handle = tjInitDecompress();
    if (not handle)
        return Error::outOfMemory("could not initialize turbojpeg");
    Defer _ = [&] {
        tjDestroy(handle);
    };

    int width = 0, height = 0, subsamp = 0, colorspace = 0;
    if (tjDecompressHeader3(handle, bytes.buf(), bytes.len(), &width, &height, &subsamp, &colorspace) < 0)
        return _error(handle);

    if (colorspace == TJCS_CMYK or colorspace == TJCS_YCCK)
        return _decodeCmyk(handle, bytes, width, height);

    auto img = Gfx::Image::alloc({width, height}, Gfx::RGBA8888);
    if (tjDecompress2(handle, bytes.buf(), bytes.len(), img->mutBytes().buf(), width, img->stride(), height, TJPF_RGBA, 0) < 0)
        return _error(handle);

    return Ok(img);
}

static int _format(Gfx::Format fmt) {
    if (fmt == Gfx::BGRA8888)
        return TJPF_BGRA;
    if (fmt == Gfx::GREYSCALE8)
        return TJPF_GRAY;
    return TJPF_RGBA;
}

static Res<> _encode(Gfx::Pixels pixels, Io::BEmit& e, Saver const& props) {
    tjhandle handle = tjInitCompress();
    if (not handle)
        return Error::outOfMemory("could not initialize turbojpeg");
    Defer _ = [&] {
        tjDestroy(handle);
    };

    int format = _format(pixels.fmt());
    int quality = clamp(props.quality, Saver::MIN_QUALITY, Saver::MAX_QUALITY) * 99 + 1;
    int subsamp = format == TJPF_GRAY ? TJSAMP_GRAY : TJSAMP_420;

    unsigned char* buf = nullptr;
    unsigned long size = 0;
    Defer _buf = [&] {
        tjFree(buf);
    };

    if (tjCompress2(handle, pixels.bytes().buf(), pixels.width(), pixels.stride(), pixels.height(), format, &buf, &size, subsamp, quality, 0) < 0)
        return _error(handle);

    return e.writeBytes({buf, size});
}

static Codec _codec{
    "libjpeg-turbo",
    Ref::Uti::PUBLIC_JPEG,
    Codec::MAGIC,
    Codec::SYSTEM,
    _sniff,
    _decode,
    _encode,
};

} // namespace Karm::Image::LibjpegTurbo
