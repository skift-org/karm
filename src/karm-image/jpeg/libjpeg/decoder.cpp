module;

// clang-format off
#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>
// clang-format on

module Karm.Image;

import Karm.Core;
import Karm.Math;

import :jpeg._embed;

namespace Karm::Image::Jpeg::_Embed {

struct ErrorManager {
    jpeg_error_mgr pub;
    jmp_buf setjmpBuffer;
};

struct LibJpegDecoder : Decoder {
    struct jpeg_decompress_struct _cinfo;
    ErrorManager _errorManager;
    Metadata _metadata;

    static Res<Box<Decoder>> init(Bytes slice) {
        auto decoder = makeBox<LibJpegDecoder>();

        decoder->_cinfo.err = jpeg_std_error(&decoder->_errorManager.pub);
        decoder->_errorManager.pub.error_exit = [](j_common_ptr cinfo) {
            auto* err = reinterpret_cast<ErrorManager*>(cinfo->err);
            longjmp(err->setjmpBuffer, 1);
        };

        if (setjmp(decoder->_errorManager.setjmpBuffer)) {
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

        decoder->_metadata = Metadata{
            .size = Math::Vec2i{decoder->_cinfo.image_width, decoder->_cinfo.image_height},
            .fmt = fmt,
        };

        return Ok(static_cast<Box<Decoder>>(std::move(decoder)));
    }

    ~LibJpegDecoder() override {
        jpeg_destroy_decompress(&_cinfo);
    }

    Metadata metadata() override {
        return _metadata;
    }

    Res<> decode(Gfx::MutPixels pixels) override {
        if (setjmp(_errorManager.setjmpBuffer)) {
            return Error::other();
        }

        jpeg_start_decompress(&_cinfo);

        auto rowStride = _cinfo.output_width * _cinfo.output_components;

        if (pixels.stride() != rowStride || pixels.height() != _cinfo.output_height) {
            return Error::invalidInput();
        }

        while (_cinfo.output_scanline < _cinfo.output_height) {
            JSAMPROW row = pixels.mutBytes().buf() + _cinfo.output_scanline * pixels.stride();
            jpeg_read_scanlines(&_cinfo, &row, 1);
        }

        return Ok();
    }
};

Res<Box<Decoder>> createJpegDecoder(Bytes slice) {
    return LibJpegDecoder::init(slice);
}

} // namespace Karm::Image::Jpeg::_Embed
