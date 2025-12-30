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

    LibJpegDecoder(const struct jpeg_decompress_struct& cinfo, ErrorManager const& error_manager, Metadata const& metadata)
        : _cinfo(cinfo),
          _errorManager(error_manager),
          _metadata(metadata) {}


    static Res<Box<Decoder>> init(Bytes slice) {
        struct jpeg_decompress_struct cinfo;
        ErrorManager errorManager;

        cinfo.err = jpeg_std_error(&errorManager.pub);
        errorManager.pub.error_exit = [](j_common_ptr cinfo) {
            auto* err = reinterpret_cast<ErrorManager*>(cinfo->err);
            longjmp(err->setjmpBuffer, 1);
        };

        if (setjmp(errorManager.setjmpBuffer)) {
            return Error::other();
        }

        jpeg_create_decompress(&cinfo);

        jpeg_mem_src(&cinfo, slice.buf(), slice.len());

        if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
            return Error::invalidData("jpeg header is invalid");
        }

        Gfx::Fmt fmt = Gfx::RGB888;
        cinfo.out_color_space = JCS_RGB;

        if (cinfo.jpeg_color_space == JCS_GRAYSCALE) {
            fmt = Gfx::GREYSCALE8;
            cinfo.out_color_space = JCS_GRAYSCALE;
        }

        auto metadata = Metadata{
            .size = Math::Vec2i{cinfo.image_width, cinfo.image_height},
            .fmt = fmt,
        };

        Box<Decoder> decoder = makeBox<LibJpegDecoder>(cinfo, errorManager, metadata);
        return Ok(std::move(decoder));
    }

    ~LibJpegDecoder() override {
        jpeg_destroy_decompress(&_cinfo);
    }

    Metadata metadata() override {
        return _metadata;
    }

    Res<> decode(Gfx::MutPixels pixels) override {
        if (pixels.fmt() != metadata().fmt || pixels.size() != metadata().size) {
            return Error::invalidInput("incompatible output pixels");
        }

        if (setjmp(_errorManager.setjmpBuffer)) {
            return Error::other();
        }

        jpeg_start_decompress(&_cinfo);

        if (pixels.stride() != _cinfo.output_width * _cinfo.output_components) {
            return Error::other("unexpected stride");
        }

        if (pixels.stride() != _cinfo.output_height) {
            return Error::other("unexpected height");
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
