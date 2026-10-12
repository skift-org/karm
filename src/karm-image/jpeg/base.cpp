module;

#include <karm/macros>

export module Karm.Image:jpeg.base;

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Logger;

namespace Karm::Image::Jpeg {

export constexpr Array<usize, 64> ZIGZAG = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

// MARK: Marker Constants ------------------------------------------------------

// Table B.1 Marker code assignments
// https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=36
export struct Marker {
    enum Code : u8 {
        NIL = 0x00,

        // Start of Frame markers, non-differential, Huffman coding
        SOF0 = 0xC0, // Baseline DCT
        SOF1 = 0xC1, // Extended sequential DCT
        SOF2 = 0xC2, // Progressive DCT
        SOF3 = 0xC3, // Lossless (sequential)

        // Start of Frame markers, differential, Huffman coding
        SOF5 = 0xC5, // Differential sequential DCT
        SOF6 = 0xC6, // Differential progressive DCT
        SOF7 = 0xC7, // Differential lossless (sequential)

        // Start of Frame markers, non-differential, arithmetic coding
        SOF9 = 0xC9,  // Extended sequential DCT
        SOF10 = 0xCA, // Progressive DCT
        SOF11 = 0xCB, // Lossless (sequential)

        // Start of Frame markers, differential, arithmetic coding
        SOF13 = 0xCD, // Differential sequential DCT
        SOF14 = 0xCE, // Differential progressive DCT
        SOF15 = 0xCF, // Differential lossless (sequential)

        // Define Huffman Table(s)
        DHT = 0xC4,

        // JPEG extensions
        JPG = 0xC8,

        // Define Arithmetic Coding Conditioning(s)
        DAC = 0xCC,

        // Restart interval Markers
        RST0 = 0xD0,
        RST1 = 0xD1,
        RST2 = 0xD2,
        RST3 = 0xD3,
        RST4 = 0xD4,
        RST5 = 0xD5,
        RST6 = 0xD6,
        RST7 = 0xD7,

        // Other Markers
        SOI = 0xD8, // Start of Image
        EOI = 0xD9, // End of Image
        SOS = 0xDA, // Start of Scan
        DQT = 0xDB, // Define Quantization Table(s)
        DNL = 0xDC, // Define Number of Lines
        DRI = 0xDD, // Define Restart Interval
        DHP = 0xDE, // Define Hierarchical Progression
        EXP = 0xDF, // Expand Reference Component(s)

        // APPN Markers
        APP0 = 0xE0,
        APP1 = 0xE1,
        APP2 = 0xE2,
        APP3 = 0xE3,
        APP4 = 0xE4,
        APP5 = 0xE5,
        APP6 = 0xE6,
        APP7 = 0xE7,
        APP8 = 0xE8,
        APP9 = 0xE9,
        APP10 = 0xEA,
        APP11 = 0xEB,
        APP12 = 0xEC,
        APP13 = 0xED,
        APP14 = 0xEE,
        APP15 = 0xEF,

        // Misc Markers
        JPG0 = 0xF0,
        JPG1 = 0xF1,
        JPG2 = 0xF2,
        JPG3 = 0xF3,
        JPG4 = 0xF4,
        JPG5 = 0xF5,
        JPG6 = 0xF6,
        JPG7 = 0xF7,
        JPG8 = 0xF8,
        JPG9 = 0xF9,
        JPG10 = 0xFA,
        JPG11 = 0xFB,
        JPG12 = 0xFC,
        JPG13 = 0xFD,
        COM = 0xFE,
        TEM = 0x01,
    };

    Code code;

    constexpr Marker(Code code) : code(code) {}

    explicit constexpr Marker(u8 raw) : code(static_cast<Code>(raw)) {}

    constexpr operator Code() const { return code; }

    constexpr bool startsFrame() const {
        return SOF0 <= code and code <= SOF15 and
               code != DHT and code != JPG and code != DAC;
    }

    constexpr bool restarts() const {
        return RST0 <= code and code <= RST7;
    }

    constexpr bool application() const {
        return APP0 <= code and code <= APP15;
    }

    constexpr bool standalone() const {
        return code == SOI or code == EOI or code == TEM or restarts();
    }

    Str _name() const {
        switch (code) {
        case NIL: return "NIL";
        case DHT: return "DHT";
        case JPG: return "JPG";
        case DAC: return "DAC";
        case SOI: return "SOI";
        case EOI: return "EOI";
        case SOS: return "SOS";
        case DQT: return "DQT";
        case DNL: return "DNL";
        case DRI: return "DRI";
        case DHP: return "DHP";
        case EXP: return "EXP";
        case COM: return "COM";
        case TEM: return "TEM";
        default: return "";
        }
    }

    void repr(Io::Emit& e) const {
        if (startsFrame())
            e("SOF{}", code - SOF0);
        else if (restarts())
            e("RST{}", code - RST0);
        else if (application())
            e("APP{}", code - APP0);
        else if (JPG0 <= code and code <= JPG13)
            e("JPG{}", code - JPG0);
        else if (auto name = _name(); name.len())
            e("{}", name);
        else
            e("0x{:02x}", (u8)code);
    }
};

// MARK: Reader/Writers --------------------------------------------------------

export struct BitWriter {
    Io::BEmit& e;

    u8 _bit = 0;
    u8 _bitLen = 0;

    static constexpr usize OUT_CAP = 32;
    Array<u8, OUT_CAP> _out;
    usize _outLen = 0;

    always_inline BitWriter(Io::BEmit& e) : e(e) {}

    always_inline Res<> putByte(u8 b) {
        _out[_outLen++] = b;
        if (_outLen == OUT_CAP)
            return flushBytes();
        return Ok();
    }

    always_inline Res<> flushBytes() {
        if (_outLen) {
            try$(e.writeBytes(sub(_out, 0, _outLen)));
            _outLen = 0;
        }
        return Ok();
    }

    always_inline Res<> writeBit(u8 bit) {
        _bit |= (bit & 1) << (7 - _bitLen);
        _bitLen = (_bitLen + 1) & 7;

        if (_bitLen == 0) {
            try$(putByte(_bit));
            if (_bit == 0xFF)
                try$(putByte(0));
            _bit = 0;
        }
        return Ok();
    }

    always_inline Res<> writeBits(u64 bits, usize len) {
        for (usize i = 1; i <= len; ++i)
            try$(writeBit(bits >> (len - i)));
        return Ok();
    }

    always_inline Res<> flushBitByte() {
        if (_bitLen) {
            try$(putByte(_bit));
            if (_bit == 0xFF)
                try$(putByte(0));
            _bit = 0;
            _bitLen = 0;
        }
        return Ok();
    }
};

export struct BitReader {
    Io::BScan& s;

    u8 _buf = 0;
    u8 _len = 0;

    always_inline BitReader(Io::BScan& s) : s(s) {}

    always_inline void reset() {
        _buf = 0x0;
        _len = 0x0;
    }

    always_inline Res<> fill() {
        if (_len)
            return Ok();

        if (s.ended()) {
            logError("jpeg: unexpected end of entropy-coded data");
            return Error::invalidData("unexpected end of entropy-coded data");
        }

        _buf = s.nextU8be();
        while (_buf == 0xFF) {
            u8 marker = s.peekU8be();

            while (marker == 0xFF) {
                s.nextU8be();
                marker = s.peekU8be();
            }

            if (marker == 0x00) {
                s.nextU8be();
                break;
            } else if (Marker{marker}.restarts()) {
                s.nextU8be();
                _buf = s.nextU8be();
            } else {
                logError("jpeg: invalid marker {:02x}", marker);
                return Error::invalidData("invalid marker");
            }
        }

        _len = 8;
        return Ok();
    }

    always_inline Res<u8> nextBit() {
        try$(fill());
        u8 bit = _buf >> 7;
        _buf <<= 1;
        _len--;
        return Ok(bit);
    }

    always_inline Res<usize> nextBits(u8 n) {
        usize res = 0;
        while (n--) {
            res = (res << 1) | try$(nextBit());
        }
        return Ok(res);
    }
};

// MARK: Blocks ----------------------------------------------------------------

export using Block = Array<short, 64>;

export usize mcuWidth(Gfx::Pixels pixels) {
    return ceilDiv(pixels.size().x, 8);
}

export usize mcuHeight(Gfx::Pixels pixels) {
    return ceilDiv(pixels.size().y, 8);
}

// MARK: Quantization Tables ---------------------------------------------------

export using Quant = Array<usize, 64>;

export void quantize(Block& block, Quant const& quant) {
    for (usize i = 0; i < 64; ++i) {
        block[i] /= quant[i];
    }
}

export void dequantize(Block& block, Quant const& quant) {
    for (usize i = 0; i < 64; ++i) {
        block[i] *= quant[i];
    }
}

// MARK: Huffman Tables --------------------------------------------------------

export struct Huff {
    Array<u8, 17> offs = {};
    Array<u8, 162> syms = {};
    Opt<Array<usize, 256>> _codes = {};

    Array<usize, 256> const& codes() {
        if (not _codes) {
            _codes.emplace();
            usize code = 0;
            for (usize i = 0; i < 16; ++i) {
                for (usize j = offs[i]; j < offs[i + 1]; ++j) {
                    (*_codes)[j] = code;
                    ++code;
                }
                code <<= 1;
            }
        }
        return *_codes;
    }

    Res<u8> next(BitReader& bs) {
        usize code = 0;
        for (usize i = 0; i < 16; ++i) {
            code = (code << 1) | try$(bs.nextBit());
            for (usize j = offs[i]; j < offs[i + 1]; ++j) {
                if (code == codes()[j]) {
                    return Ok(syms[j]);
                }
            }
        }

        logError("jpeg: invalid huffman code {:x}", code);
        return Error::invalidData("invalid huffman code");
    }

    bool getCode(u8 symbol, usize& code, usize& codeLength) {
        for (usize i = 0; i < 16; ++i) {
            for (usize j = offs[i]; j < offs[i + 1]; ++j) {
                if (symbol == syms[j]) {
                    code = codes()[j];
                    codeLength = i + 1;
                    return true;
                }
            }
        }
        return false;
    }
};

// MARK: Coefficients ---------------------------------------------------------

// F.1.2.1 Huffman encoding of DC coefficients
// https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=92
export constexpr usize additionalBits(isize value, usize len) {
    if (value < 0)
        return value + (isize)((1uz << len) - 1);
    return value;
}

// F.2.2.1 Huffman decoding of DC coefficients
// https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=108
export constexpr isize extend(usize bits, usize len) {
    if (len == 0)
        return 0;

    if (bits < (1uz << (len - 1)))
        return (isize)bits - (isize)((1uz << len) - 1);

    return (isize)bits;
}

static_assert(extend(0b000, 3) == -7);
static_assert(extend(0b011, 3) == -4);
static_assert(extend(0b100, 3) == 4);
static_assert(extend(0b111, 3) == 7);

static constexpr bool _coefficientsRoundtrip() {
    for (usize len = 1; len <= 11; len++) {
        for (isize v = 1 << (len - 1); v < (1 << len); v++) {
            if (extend(additionalBits(v, len), len) != v or
                extend(additionalBits(-v, len), len) != -v)
                return false;
        }
    }
    return true;
}

static_assert(_coefficientsRoundtrip());

export usize bitLength(int v) {
    usize length = 0;
    while (v > 0) {
        v >>= 1;
        length += 1;
    }
    return length;
}

} // namespace Karm::Image::Jpeg
