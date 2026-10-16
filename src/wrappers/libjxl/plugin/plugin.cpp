#include <jxl/decode.h>
#include <jxl/encode.h>
#include <karm/macros>

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Image;
import Karm.Ref;

namespace Karm::Image::Libjxl {

static bool _sniff(Bytes bytes) {
    auto sig = JxlSignatureCheck(bytes.buf(), bytes.len());
    return sig == JXL_SIG_CODESTREAM or sig == JXL_SIG_CONTAINER;
}

static Res<Rc<Gfx::Image>> _decode(Bytes bytes) {
    JxlDecoder* decoder = JxlDecoderCreate(nullptr);
    if (not decoder)
        return Error::outOfMemory("could not create jxl decoder");
    Defer _ = [&] {
        JxlDecoderDestroy(decoder);
    };

    if (JxlDecoderSubscribeEvents(decoder, JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING | JXL_DEC_FULL_IMAGE) != JXL_DEC_SUCCESS)
        return Error::invalidData("could not subscribe to jxl decoder events");

    if (JxlDecoderSetInput(decoder, bytes.buf(), bytes.len()) != JXL_DEC_SUCCESS)
        return Error::invalidData("could not set jxl decoder input");
    JxlDecoderCloseInput(decoder);

    JxlPixelFormat format{4, JXL_TYPE_UINT8, JXL_NATIVE_ENDIAN, 0};
    Opt<Rc<Gfx::Image>> img;

    while (true) {
        switch (JxlDecoderProcessInput(decoder)) {
        case JXL_DEC_BASIC_INFO: {
            JxlBasicInfo info;
            if (JxlDecoderGetBasicInfo(decoder, &info) != JXL_DEC_SUCCESS)
                return Error::invalidData("could not read jxl basic info");
            img = Some(Gfx::Image::alloc({(isize)info.xsize, (isize)info.ysize}, Gfx::RGBA8888));
            break;
        }

        case JXL_DEC_COLOR_ENCODING: {
            // NOTE: Gfx::Image pixels are assumed to be sRGB, this makes libjxl
            //       convert XYB images to it (non-XYB images are left as is).
            JxlColorEncoding srgb;
            JxlColorEncodingSetToSRGB(&srgb, JXL_FALSE);
            if (JxlDecoderSetPreferredColorProfile(decoder, &srgb) != JXL_DEC_SUCCESS)
                return Error::invalidData("could not set jxl output color profile");
            break;
        }

        case JXL_DEC_NEED_IMAGE_OUT_BUFFER: {
            if (not img)
                return Error::invalidData("jxl image buffer requested before basic info");
            auto out = (*img)->mutBytes();
            usize size = 0;
            if (JxlDecoderImageOutBufferSize(decoder, &format, &size) != JXL_DEC_SUCCESS or size != out.len())
                return Error::invalidData("unexpected jxl image buffer size");
            if (JxlDecoderSetImageOutBuffer(decoder, &format, out.buf(), out.len()) != JXL_DEC_SUCCESS)
                return Error::invalidData("could not set jxl image buffer");
            break;
        }

        // NOTE: Only the first frame of animated images is decoded.
        case JXL_DEC_FULL_IMAGE:
        case JXL_DEC_SUCCESS:
            if (not img)
                return Error::invalidData("jxl image has no frame");
            return Ok(*img);

        case JXL_DEC_NEED_MORE_INPUT:
            return Error::unexpectedEof("truncated jxl image");

        default:
            return Error::invalidData("could not decode jxl image");
        }
    }
}

static Codec _codec{
    "libjxl",
    Ref::Uti::PUBLIC_JXL,
    Codec::MAGIC,
    Codec::SYSTEM,
    _sniff,
    _decode,
};

} // namespace Karm::Image::Libjxl
