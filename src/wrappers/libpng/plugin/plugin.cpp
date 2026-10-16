#include <png.h>

#include <karm/macros>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Logger;
import Karm.Ref;

namespace Karm::Image::Libpng {

static Error _error(png_image const& image) {
    return Error::invalidData(String{Str::fromNullterminated(image.message)});
}

static bool _sniff(Bytes bytes) {
    return bytes.len() >= 8 and png_sig_cmp(bytes.buf(), 0, 8) == 0;
}

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    png_image image{};
    image.version = PNG_IMAGE_VERSION;

    if (not png_image_begin_read_from_memory(&image, bytes.buf(), bytes.len())) {
        png_image_free(&image);
        return _error(image);
    }

    image.format = PNG_FORMAT_RGBA;
    auto img = Gfx::Image::alloc({(isize)image.width, (isize)image.height}, Gfx::RGBA8888);
    if (not png_image_finish_read(&image, nullptr, img->mutBytes().buf(), img->stride(), nullptr)) {
        png_image_free(&image);
        return _error(image);
    }

    if (image.warning_or_error)
        logWarn("libpng: {}", Str::fromNullterminated(image.message));

    return Ok(img);
}

static png_uint_32 _format(Gfx::Format fmt) {
    if (fmt == Gfx::BGRA8888)
        return PNG_FORMAT_BGRA;
    if (fmt == Gfx::GREYSCALE8)
        return PNG_FORMAT_GRAY;
    return PNG_FORMAT_RGBA;
}

static Res<> _encode(Gfx::Pixels pixels, Io::BEmit& e, Saver const&) {
    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = pixels.width();
    image.height = pixels.height();
    image.format = _format(pixels.fmt());

    png_alloc_size_t size = 0;
    if (not png_image_write_get_memory_size(image, size, 0, pixels.bytes().buf(), pixels.stride(), nullptr))
        return _error(image);

    auto buf = Vec<u8>::filled(size, 0);
    if (not png_image_write_to_memory(&image, buf.buf(), &size, 0, pixels.bytes().buf(), pixels.stride(), nullptr))
        return _error(image);

    return e.writeBytes(sub(buf, 0, size));
}

static Codec _codec{"libpng", Ref::Uti::PUBLIC_PNG, Codec::MAGIC, Codec::SYSTEM, _sniff, _decode, _encode};

} // namespace Karm::Image::Libpng
