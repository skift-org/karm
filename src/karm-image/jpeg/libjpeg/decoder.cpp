module;

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

module Karm.Image;

import Karm.Core;
import Karm.Math;

import :jpeg._embed;

namespace Karm::Image::Jpeg::_Embed {
struct ErrorManager {
    jpeg_error_mgr pub;
    jmp_buf setjmpBuffer;
};

static void errorExit(j_common_ptr cinfo) {
    auto* err = (ErrorManager*)cinfo->err;
    longjmp(err->setjmpBuffer, 1);
}

struct JpegDecoder : Decoder {
    struct jpeg_decompress_struct _cinfo;
    ErrorManager _errorManager;
    Metadata _metadata{};

    static Res<Box<Decoder>> init(Bytes slice) {
        auto decoder = makeBox<JpegDecoder>();

        decoder->_cinfo.err = jpeg_std_error(&decoder->_errorManager.pub);
        decoder->_errorManager.pub.error_exit = errorExit;

        if (setjmp(decoder->_errorManager.setjmpBuffer)) {
            jpeg_destroy_decompress(&decoder->_cinfo);
            // FIXME: Introduce library error
            return Error::other();
        }

        jpeg_create_decompress(&decoder->_cinfo);

        jpeg_mem_src(&decoder->_cinfo, slice.buf(), slice.len());

        if (jpeg_read_header(&decoder->_cinfo, TRUE) != JPEG_HEADER_OK) {
            return Error::invalidData("jpeg header is invalid");
        }
        // FIXME: Support CMYK
        Opt<Gfx::Fmt> fmt;
        switch (decoder->_cinfo.jpeg_color_space) {
        case JCS_GRAYSCALE:
            fmt = Gfx::GREYSCALE8;
            break;
        case JCS_YCbCr:
        case JCS_RGB:
            fmt = Gfx::RGB888;
            break;
        default:
            break;
        }

        jpeg_destroy_decompress(&decoder->_cinfo);

        decoder->_metadata = Metadata{
            .size = Math::Vec2i{decoder->_cinfo.image_width, decoder->_cinfo.image_height},
            .fmt = fmt,
        };

        return Ok(static_cast<Box<Decoder>>(std::move(decoder)));
    }

    ~JpegDecoder() override {
        jpeg_destroy_decompress(&_cinfo);
    }

    Metadata const& metadata() override {
        return _metadata;
    }

    Res<> decode(Gfx::MutPixels pixels) override {
        return Error::notImplemented();
    }
};

Res<Box<Decoder>> createJpegDecoder(Bytes slice) {
    return JpegDecoder::init(slice);
}

} // namespace Karm::Image::Jpeg::_Embed
