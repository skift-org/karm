module;

#include <karm-gfx/canvas.h>
#include <karm-logger/logger.h>

export module Karm.Image:png.decoder;

import Karm.Core;
import Karm.Archive;
import Karm.Debug;

namespace Karm::Image::Png {

static Debug::Flag debugPng{"png"};

enum struct ColorType : u8 {
    GREYSCALE = 0,
    TRUECOLOR = 2,
    INDEXED = 3,
    GREYSCALE_ALPHA = 4,
    TRUECOLOR_ALPHA = 6,

    _LEN
};

enum struct CompressionMethod : u8 {
    DEFLATE = 0,

    _LEN
};

enum struct FilterMethod : u8 {
    STANDARD = 0,

    _LEN
};

enum struct Filter : u8 {
    NONE,
    SUB,
    UP,
    AVERAGE,
    PAETH,

    _LEN
};

enum struct InterlacingMethod : u8 {
    NULL,
    ADAM7,

    _LEN,
};

static constexpr Array<u8, 8> SIG = {
    0x89, 0x50, 0x4E, 0x47,
    0x0D, 0x0A, 0x1A, 0x0A
};

static constexpr Str IHDR = "IHDR";
static constexpr Str PLTE = "PLTE";
static constexpr Str IDAT = "IDAT";
static constexpr Str IEND = "IEND";

export struct Decoder {
    Bytes _slice;

    Bytes sig() {
        return begin().nextBytes(8);
    }

    static bool sniff(Bytes slice) {
        return slice.len() >= 8 and sub(slice, 0, 8) == SIG;
    }

    static Res<Decoder> init(Bytes slice) {
        Decoder dec{slice};

        if (dec.sig() != SIG)
            return Error::invalidData("invalid signature");

        // Preloading the header this way all the image
        try$(dec._loaderHeader());

        return Ok(dec);
    }

    Decoder(Bytes slice)
        : _slice(slice) {}

    Io::BScan begin() const {
        return _slice;
    }

    auto iterChunks() {
        auto s = begin();
        s.skip(8);

        struct Chunk {
            Str sig;
            usize len;
            Bytes data;
            u32 crc32;
        };

        return Iter{[s] mutable -> Opt<Chunk> {
            if (s.ended())
                return NONE;

            Chunk c;

            c.len = s.nextI32be();
            c.sig = s.nextStr(4);
            c.data = s.nextBytes(c.len);
            c.crc32 = s.nextI32be();

            return c;
        }};
    }

    // MARK: Header ------------------------------------------------------------

    u32 _width = 0;
    u32 _height = 0;
    u8 _bitDepth = 0;
    ColorType _colorType;
    CompressionMethod _compressionMethod;
    FilterMethod _filterMethod;
    InterlacingMethod _interlacingMethod;

    isize width() const {
        return _width;
    }

    isize height() const {
        return _height;
    }

    Res<> _loaderHeader() {
        auto maybeIhdr = iterChunks().next();
        if (not maybeIhdr)
            return Error::invalidData("missing first chunk");

        if (maybeIhdr->sig != IHDR)
            return Error::invalidData("missing image header");

        Io::BScan ihdr = maybeIhdr->data;
        try$(_handleIhdr(ihdr));
        return Ok();
    }

    Res<> _handleIhdr(Io::BScan& s) {
        _width = s.nextU32be();
        _height = s.nextU32be();
        _bitDepth = s.nextU8be();
        _colorType = static_cast<ColorType>(s.nextU8be());
        _compressionMethod = static_cast<CompressionMethod>(s.nextU8be());
        _filterMethod = static_cast<FilterMethod>(s.nextU8be());
        _interlacingMethod = static_cast<InterlacingMethod>(s.nextU8be());

        return Ok();
    }

    // MARK: Palette -----------------------------------------------------------

    Vec<Gfx::Color> _palette;

    Res<> _handlePlte(Io::BScan& s) {
        while (not s.ended()) {
            u8 red = s.nextU8le();
            u8 green = s.nextU8le();
            u8 blue = s.nextU8le();
            _palette.pushBack(Gfx::Color{red, green, blue});
        }

        return Ok();
    }

    // MARK: Image Data --------------------------------------------------------

    Io::BufferWriter _compressedData;

    Res<> _handleIdat(Io::BScan& s) {
        try$(_compressedData.write(s.remBytes()));

        return Ok();
    }

    // MARK: Image End ---------------------------------------------------------

    bool _ended = false;

    Res<> _handleIend(Io::BScan&) {
        if (_ended)
            return Error::invalidData("multiple image end");
        _ended = true;

        return Ok();
    }

    // MARK: Decoding ----------------------------------------------------------

    Res<> _decodeScanline(Io::BScan& s, Gfx::MutPixels out, usize scanline) {
        u8 filter = s.nextU8be();
        (void)filter;
        for (usize i : range(_width)) {
            auto r = s.nextU8be();
            auto g = s.nextU8be();
            auto b = s.nextU8be();
            out.store(Math::Vec2u{i, scanline}.cast<isize>(), Gfx::Color{r, g, b});
        }
        return Ok();
    }

    Res<> decode(Gfx::MutPixels out) {
        for (auto chunk : iterChunks()) {
            Io::BScan data = chunk.data;
            if (chunk.sig == IHDR)
                try$(_handleIhdr(data));
            else if (chunk.sig == PLTE)
                try$(_handlePlte(data));
            else if (chunk.sig == IDAT)
                try$(_handleIdat(data));
            else if (chunk.sig == IEND)
                try$(_handleIend(data));
            else
                logWarnIf(debugPng, "unknow chunk {#}", chunk.sig);
        }

        if (not _ended)
            return Error::invalidData("missing image end");

        logDebug("{}", *this);

        if (_bitDepth != 8)
            return Error::invalidData("unsupported bit depth");

        if (_compressionMethod != CompressionMethod::DEFLATE)
            return Error::invalidData("unsupported compression methode");

        if (_filterMethod != FilterMethod::STANDARD)
            return Error::invalidData("unsupported filter methode");

        if (_interlacingMethod != InterlacingMethod::NULL)
            return Error::invalidData("unsupported interlacing methode");

        auto imageData = try$(Archive::zlibDecompress(_compressedData.bytes()));

        Io::BScan s = bytes(imageData);
        for (usize const scanline : range(_height)) {
            try$(_decodeScanline(s, out, scanline));
        }

        return Ok();
    }

    void repr(Io::Emit& e) const {
        e("PNG image");

        e.indentNewline();

        e.ln("width: {}", _width);
        e.ln("height: {}", _height);
        e.ln("bit depth: {}", _bitDepth);
        e.ln("color type: {}", _colorType);
        e.ln("compression method: {}", _compressionMethod);
        e.ln("filter method: {}", _filterMethod);
        e.ln("interlacing method: {}", _interlacingMethod);

        e.deindent();
    }
};

} // namespace Karm::Image::Png
