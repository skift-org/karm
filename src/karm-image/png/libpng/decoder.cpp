module;

#include <libpng/png.h>
#include <setjmp.h>
#include <karm-core/macros.h>

module Karm.Image;

import Karm.Core;
import Karm.Math;

import :png._embed;

namespace Karm::Image::Png::_Embed {

struct LibPngDecoder : Decoder {
    png_structp _png;
    png_infop _info;
    Metadata _metadata;
    Box<Io::Reader> _reader;

    LibPngDecoder(png_structp png, png_infop info, Metadata const& metadata, Box<Io::Reader>&& reader)
        : _png(png),
          _info(info),
          _metadata(metadata),
          _reader(std::move(reader)) {}

    static Res<Box<Decoder>> init(Bytes slice) {
        auto* png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (not png) {
            return Error::other(); // FIXME
        }

        auto* info = png_create_info_struct(png);
        if (not info) {
            return Error::other(); // FIXME
        }

        if (setjmp(png_jmpbuf(png))) {
            return Error::other(); // FIXME
        }

        // dependency on this:
        auto reader = makeBox<Io::BufReader>(slice);

        png_set_read_fn(png, reader._ptr, [](png_structp pngPtr, png_bytep outBytes, size_t byteCount) {
            auto* reader = static_cast<Io::BufReader*>(png_get_io_ptr(pngPtr));

            auto bytesRead = reader->read(MutBytes{outBytes, byteCount});
            if (bytesRead.error()) {
                png_error(pngPtr, bytesRead.error()->msg());
            }

            if (bytesRead.unwrap() < byteCount) {
                png_error(pngPtr, "read past end of buffer");
            }
        });

        png_read_info(png, info);

        if (png_get_valid(png, info, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png);

        if (png_get_color_type(png, info) == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png);

        if (png_get_bit_depth(png, info) == 16) {
            png_set_strip_16(png);
        }

        png_read_update_info(png, info);

        auto resolveFormat = [&]() -> Res<Gfx::Fmt> {
            switch (png_get_color_type(png, info)) {
            case PNG_COLOR_TYPE_RGB:
                return Ok(Gfx::RGB888);
            case PNG_COLOR_TYPE_RGBA:
                return Ok(Gfx::RGBA8888);
            case PNG_COLOR_TYPE_GRAY:
                return Ok(Gfx::GREYSCALE8);
            case PNG_COLOR_TYPE_GA:
                return Ok(Gfx::GA88);
            default:
                return Error::invalidData("png: unknown color space");
            }
        };

        auto metadata = Metadata{
            .size = {
                png_get_image_width(png, info),
                png_get_image_height(png, info)
            },
            .fmt = try$(resolveFormat()),
        };

        Box<Decoder> decoder = makeBox<LibPngDecoder>(png, info, metadata, std::move(reader));
        return Ok(std::move(decoder));
    }

    ~LibPngDecoder() override {
        png_destroy_read_struct(&_png, &_info, nullptr);
    }

    Metadata metadata() override {
        return _metadata;
    }

    Res<> decode(Gfx::MutPixels pixels) override {
        if (pixels.fmt() != metadata().fmt || pixels.size() != metadata().size) {
            return Error::invalidInput("trying to decode image to incompatible surface");
        }

        if (setjmp(png_jmpbuf(_png))) {
            return Error::invalidData("png: failed to decode");
        }

        auto rows = Buf<unsigned char*>::init(pixels.height());
        for (isize y = 0; y < pixels.height(); y++) {
            rows[y] = pixels.mutBytes().buf() + y * pixels.stride();
        }

        png_read_image(_png, rows.buf());

        return Ok();
    }
};

Res<Box<Decoder>> createPngDecoder(Bytes slice) {
    return LibPngDecoder::init(slice);
}

} // namespace Karm::Image::Jpeg::_Embed
