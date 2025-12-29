module;

// clang-format off
#include <libpng/png.h>
#include <setjmp.h>
// clang-format on

module Karm.Image;

import Karm.Core;
import Karm.Math;

import :png._embed;

namespace Karm::Image::Png::_Embed {

struct LibPngDecoder : Decoder {
    png_structp _png;
    png_infop _info;
    Metadata _metadata;
    Io::BufReader _reader;

    LibPngDecoder(Io::BufReader reader)
        : _reader(reader) {}

    static Res<Box<Decoder>> init(Bytes slice) {
        auto decoder = makeBox<LibPngDecoder>(slice);

        decoder->_png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (not decoder->_png) {
            return Error::other(); // FIXME
        }

        decoder->_info = png_create_info_struct(decoder->_png);
        if (not decoder->_info) {
            return Error::other(); // FIXME
        }

        if (setjmp(png_jmpbuf(decoder->_png))) {
            return Error::other(); // FIXME
        }

        png_set_read_fn(decoder->_png, &decoder->_reader, [](png_structp pngPtr, png_bytep outBytes, size_t byteCount) {
            auto* reader = static_cast<Io::BufReader*>(png_get_io_ptr(pngPtr));

            auto bytesRead = reader->read(MutBytes{outBytes, byteCount});
            if (bytesRead.error()) {
                png_error(pngPtr, bytesRead.error()->msg());
            }

            if (bytesRead.unwrap() < byteCount) {
                png_error(pngPtr, "read past end of buffer");
            }
        });

        png_read_info(decoder->_png, decoder->_info);

        Opt<Gfx::Fmt> fmt = NONE;
        if (png_get_bit_depth(decoder->_png, decoder->_info) == 8) {
            switch (png_get_color_type(decoder->_png, decoder->_info)) {
            case PNG_COLOR_TYPE_RGB:
                fmt = Gfx::RGB888;
                break;
            case PNG_COLOR_TYPE_RGBA:
                fmt = Gfx::RGBA8888;
                break;
            case PNG_COLOR_TYPE_GRAY:
                fmt = Gfx::GREYSCALE8;
                break;
            default:
                break;
            }
        }

        decoder->_metadata = Metadata{
            .size = {
                png_get_image_width(decoder->_png, decoder->_info),
                png_get_image_height(decoder->_png, decoder->_info)
            },
            .fmt = fmt
        };

        return Ok(static_cast<Box<Decoder>>(std::move(decoder)));
    }

    ~LibPngDecoder() override {
        png_destroy_read_struct(&_png, &_info, nullptr);
    }

    Metadata metadata() override {
        return _metadata;
    }

    Res<> decode(Gfx::MutPixels pixels) override {
        if (setjmp(png_jmpbuf(_png))) {
            return Error::other(); // FIXME
        }

        // FIXME: Handle 16bit depth & other conversions
        png_read_update_info(_png, _info);

        auto rowStride = png_get_rowbytes(_png, _info);

        if (pixels.stride() != rowStride || pixels.height() != _metadata.size.y) {
            return Error::invalidInput();
        }

        for (isize y = 0; y < pixels.height(); y++) {
            png_read_row(_png, pixels.mutBytes().buf() + y * pixels.stride(), nullptr);
        }

        return Ok();
    }
};

Res<Box<Decoder>> createPngDecoder(Bytes slice) {
    return LibPngDecoder::init(slice);
}

} // namespace Karm::Image::Jpeg::_Embed
