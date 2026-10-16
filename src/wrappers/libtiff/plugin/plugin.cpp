#include <stdio.h>
#include <string.h>
#include <tiffio.h>

#include <karm/macros>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;

namespace Karm::Image::Libtiff {

struct Source {
    Bytes data;
    usize pos = 0;
    Opt<String> error = NONE;
};

static tmsize_t _read(thandle_t handle, void* buf, tmsize_t len) {
    auto& source = *static_cast<Source*>(handle);
    usize n = min((usize)len, source.data.len() - source.pos);
    memcpy(buf, source.data.buf() + source.pos, n);
    source.pos += n;
    return n;
}

static tmsize_t _write(thandle_t, void*, tmsize_t) {
    return 0;
}

static toff_t _seek(thandle_t handle, toff_t off, int whence) {
    auto& source = *static_cast<Source*>(handle);
    toff_t base = 0;
    if (whence == SEEK_CUR)
        base = source.pos;
    else if (whence == SEEK_END)
        base = source.data.len();

    if (base + off > source.data.len())
        return (toff_t)-1;
    source.pos = base + off;
    return source.pos;
}

static int _close(thandle_t) {
    return 0;
}

static toff_t _size(thandle_t handle) {
    return static_cast<Source*>(handle)->data.len();
}

static int _map(thandle_t, void**, toff_t*) {
    return 0;
}

static void _unmap(thandle_t, void*, toff_t) {
}

// NOTE: Per handle error handlers were added in libtiff 4.5.0, along with
//       the version macros, older versions report errors on stderr.
#ifdef TIFFLIB_MAJOR_VERSION
#    define HAS_OPEN_OPTIONS
#endif

#ifdef HAS_OPEN_OPTIONS
static int _onError(TIFF*, void* userData, char const* module, char const* fmt, va_list args) {
    auto& source = *static_cast<Source*>(userData);
    char buf[256];
    vsnprintf(buf, sizeof(buf), fmt, args);
    source.error = Some(Io::format("{}: {}", Str::fromNullterminated(module), Str::fromNullterminated(buf)));
    return 1;
}

static int _onWarning(TIFF*, void*, char const*, char const*, va_list) {
    return 1;
}
#endif

static bool _sniff(Bytes bytes) {
    if (bytes.len() < 4)
        return false;
    // NOTE: Accepts both classic TIFF (42) and BigTIFF (43).
    bool le = bytes[0] == 'I' and bytes[1] == 'I' and (bytes[2] == 42 or bytes[2] == 43) and bytes[3] == 0;
    bool be = bytes[0] == 'M' and bytes[1] == 'M' and bytes[2] == 0 and (bytes[3] == 42 or bytes[3] == 43);
    return le or be;
}

static Error _error(Source const& source, Str fallback) {
    if (source.error)
        return Error::invalidData(*source.error);
    return Error::invalidData(fallback);
}

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    Source source{bytes};

#ifdef HAS_OPEN_OPTIONS
    TIFFOpenOptions* opts = TIFFOpenOptionsAlloc();
    if (not opts)
        return Error::outOfMemory("could not allocate tiff options");
    TIFFOpenOptionsSetErrorHandlerExtR(opts, _onError, &source);
    TIFFOpenOptionsSetWarningHandlerExtR(opts, _onWarning, &source);
    TIFF* tif = TIFFClientOpenExt("memory", "rm", &source, _read, _write, _seek, _close, _size, _map, _unmap, opts);
    TIFFOpenOptionsFree(opts);
#else
    TIFF* tif = TIFFClientOpen("memory", "rm", &source, _read, _write, _seek, _close, _size, _map, _unmap);
#endif

    if (not tif)
        return _error(source, "could not open tiff image");
    Defer _ = [&] {
        TIFFClose(tif);
    };

    uint32_t width = 0, height = 0;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height);

    auto raster = Vec<uint32_t>::filled((usize)width * height);
    if (not TIFFReadRGBAImageOriented(tif, width, height, raster.buf(), ORIENTATION_TOPLEFT, 0))
        return _error(source, "could not decode tiff image");

    auto img = Gfx::Image::alloc({(isize)width, (isize)height}, Gfx::RGBA8888);
    auto pixels = img->mutPixels();
    for (isize y = 0; y < (isize)height; y++) {
        for (isize x = 0; x < (isize)width; x++) {
            uint32_t abgr = raster[y * width + x];
            u8 a = TIFFGetA(abgr);
            u8 r = TIFFGetR(abgr), g = TIFFGetG(abgr), b = TIFFGetB(abgr);
            // NOTE: libtiff hands out premultiplied alpha, Gfx::Image isn't.
            if (a != 0 and a != 255) {
                r = min(255u, r * 255u / a);
                g = min(255u, g * 255u / a);
                b = min(255u, b * 255u / a);
            }
            pixels.store({x, y}, Gfx::Color::fromRgba(r, g, b, a));
        }
    }

    return Ok(img);
}

static Codec _codec{"libtiff", Ref::Uti::PUBLIC_TIFF, Codec::MAGIC, Codec::SYSTEM, _sniff, _decode};

} // namespace Karm::Image::Libtiff
