module;

#include <karm/macros>

export module Karm.Bmp:decoder;

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Math;
import Karm.Logger;

// BMP image decoder
// References:
//  - https://en.wikipedia.org/wiki/BMP_file_format
//  - https://docs.microsoft.com/en-us/windows/win32/gdi/bitmap-storage
//  - http://www.ece.ualberta.ca/~elliott/ee552/studentAppNotes/2003_w/misc/bmp_file_format/bmp_file_format.htm
//  - http://www.martinreddy.net/gfx/2d/BMP.txt

namespace Karm::Bmp {

export struct Decoder {
    // MARK: Loading -----------------------------------------------------------

    static bool sniff(Bytes slice) {
        return slice.len() >= 2 and
               slice[0] == 0x42 and
               slice[1] == 0x4D;
    }

    static Res<Decoder> init(Bytes slice) {
        if (not sniff(slice)) {
            return Error::invalidData("invalid signature");
        }

        Decoder dec{};
        Io::BScan s{slice};
        try$(dec._readHeader(s));
        try$(dec._readInfoHeader(s));
        try$(dec._readPalette(s));
        try$(dec._readPixels(s));

        return Ok(dec);
    }

    // MARK: Header ------------------------------------------------------------

    usize _dataOffset;

    Res<> _readHeader(Io::BScan& s) {
        if (s.rem() < 54) {
            return Error::invalidData("image too small");
        }

        try$(s.skip(2)); // signature
        try$(s.skip(4)); // file size
        try$(s.skip(4)); // reserved
        _dataOffset = try$(s.nextI32le());

        return Ok();
    }

    isize _width;
    isize _height;
    isize _bpp;

    isize width() const {
        return Math::abs(_width);
    }

    isize height() const {
        return Math::abs(_height);
    }

    enum Compression {
        RGB = 0,
        RLE8 = 1,
        RLE4 = 2,
    } _compression;

    usize _numsColors;

    Res<> _readInfoHeader(Io::BScan& s) {
        auto start = s.tell();
        auto size = try$(s.nextU32le()); // header size
        if (size < 40) {
            return Error::invalidData("invalid header size");
        }

        _width = try$(s.nextI32le());
        _height = try$(s.nextI32le());

        auto planes = try$(s.nextI16le());
        logDebug("planes: {}", planes);
        if (planes != 1) {
            return Error::invalidData("invalid number of planes");
        }

        _bpp = try$(s.nextI16le());

        auto compression = try$(s.nextI32le());
        if (compression != RGB and compression != RLE8 and compression != RLE4) {
            return Error::invalidData("invalid compression");
        }

        try$(s.skip(4)); // image size
        try$(s.skip(4)); // x pixels per meter
        try$(s.skip(4)); // y pixels per meter
        _numsColors = try$(s.nextI32le());
        if (_numsColors == 0 and _bpp <= 8) {
            _numsColors = 1 << _bpp;
        }

        try$(s.skip(4)); // important colors
        try$(s.skip(size - (s.tell() - start)));

        return Ok();
    }

    // MARK: Palette -----------------------------------------------------------

    Vec<Gfx::Color> _palette;

    Res<> _readPalette(Io::BScan& s) {
        for (usize i = 0; i < _numsColors; ++i) {
            auto b = try$(s.nextU8le());
            auto g = try$(s.nextU8le());
            auto r = try$(s.nextU8le());
            try$(s.skip(1)); // reserved

            logDebug("palette[{}]: r: {}, g: {}, b: {}", i, r, g, b);
            _palette.pushBack(Gfx::Color{r, g, b});
        }

        return Ok();
    }

    // MARK: Pixels ------------------------------------------------------------

    Bytes _pixels;

    Res<> _readPixels(Io::BScan& s) {
        try$(s.seek(_dataOffset));
        _pixels = s.remBytes();
        return Ok();
    }

    // MARK: Decoding ----------------------------------------------------------

    Res<> decode(Gfx::MutPixels pixels) {
        Io::BScan s{_pixels};

        for (isize y = 0; y < height(); ++y) {
            for (isize x = 0; x < width(); ++x) {
                auto color = Gfx::Color{};
                if (_bpp == 1) {
                    auto bit = try$(s.nextBitbe());
                    if (bit >= _palette.len())
                        return Error::invalidData("invalid palette index");
                    color = _palette[bit];
                } else if (_bpp == 4) {
                    auto index = try$(s.nextBitsbe(4));
                    if (index >= _palette.len())
                        return Error::invalidData("invalid palette index");
                    color = _palette[index];
                } else if (_bpp == 8) {
                    auto index = try$(s.nextU8le());
                    if (index >= _palette.len()) {
                        return Error::invalidData("invalid palette index");
                    }
                    color = _palette[index];
                } else if (_bpp == 16) {
                    auto pixel = try$(s.nextU16le());
                    color.blue = (pixel & 0x1F) << 3;
                    color.green = ((pixel >> 5) & 0x1F) << 3;
                    color.red = ((pixel >> 10) & 0x1F) << 3;
                    color.alpha = 255;
                } else if (_bpp == 24) {
                    color.blue = try$(s.nextU8le());
                    color.green = try$(s.nextU8le());
                    color.red = try$(s.nextU8le());
                    color.alpha = 255;
                } else if (_bpp == 32) {
                    color.blue = try$(s.nextU8le());
                    color.green = try$(s.nextU8le());
                    color.red = try$(s.nextU8le());
                    color.alpha = 255 - try$(s.nextU8le());
                } else {
                    return Error::invalidData("invalid bpp");
                }

                Math::Vec2i pos{x, y};

                // if _width is negative, flip horizontally.
                if (_width < 0) {
                    pos.x = width() - x - 1;
                }

                // if _height is negative, flip vertically.
                if (not(_height < 0)) {
                    pos.y = height() - y - 1;
                }

                pixels.store(pos, color);
            }

            // Skip padding.
            s.alignBits();
            try$(s.skip((4 - ceilDiv(width() * _bpp, 8) % 4) % 4));
        }

        return Ok();
    }

    // MARK: Dumping -----------------------------------------------------------

    void repr(Io::Emit& e) const {
        e("BMP image");
        e.indentNewline();

        e("Header:");
        e.indentNewline();
        e.ln("data offset: {}", _dataOffset);
        e.deindent();

        e("Info header");
        e.indentNewline();
        e.ln("width: {}", _width);
        e.ln("height: {}", _height);
        e.ln("bpp: {}", _bpp);
        e.ln("compression: {}", (usize)_compression);
        e.ln("numsColors: {}", _numsColors);
        e.deindent();

        e("palette:");
        e.indentNewline();
        e.ln("{}", _palette);
        e.deindent();

        e("pixels data: {} bytes", _pixels.len());

        e.deindent();
    }
};

} // namespace Karm::Bmp
