module;

#include <karm/macros>

export module Karm.Jpeg:decoder;

import Karm.Core;
import Karm.Gfx.Pixels;
import Karm.Logger;

import :base;
import :dct;

// JPEG Image decoder
// Based on:
//  - https://github.com/klange/toaruos/blob/6a400b399ca35147f89e6bbec017716d1897ea12/lib/jpeg.c
//  - https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format
//  - https://www.w3.org/Graphics/JPEG/itu-t81.pdf
//  - https://github.com/dannye/jed/blob/master/src/decoder.cpp
//  - https://www.youtube.com/watch?v=CPT4FSkFUgs
namespace Karm::Jpeg {

using Karm::begin, Karm::end;

// B.1.1.5 Entropy-coded data segments
// https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=37
static usize _entropyCodedLen(Bytes data) {
    for (usize i = 0; i + 1 < data.len(); i++) {
        if (data[i] != 0xFF)
            continue;

        u8 next = data[i + 1];
        if (next == 0x00 or Marker{next}.restarts()) {
            i++;
            continue;
        }

        return i;
    }
    return data.len();
}

// MARK: Segments --------------------------------------------------------------

struct Segment {
    Marker marker;
    Bytes data = {};
    Bytes entropyCoded = {};
};

struct SegmentIter {
    Io::BScan s;

    Opt<Segment> next() {
        if (s.ended())
            return NONE;

        if (s.nextU8be() != 0xFF) {
            logError("jpeg: expected a marker");
            return NONE;
        }

        u8 code = s.nextU8be();
        while (code == 0xFF and not s.ended())
            code = s.nextU8be();

        Segment segment{.marker = Marker{code}};
        if (segment.marker.standalone())
            return Some(segment);

        u16 len = s.nextU16be();
        if (len < 2) {
            logError("jpeg: invalid segment length: {}", len);
            return NONE;
        }

        segment.data = s.nextBytes(len - 2);
        if (segment.marker == Marker::SOS)
            segment.entropyCoded = s.nextBytes(_entropyCodedLen(s.remBytes()));

        return Some(segment);
    }
};

// MARK: Decoder ---------------------------------------------------------------

export struct Decoder {
    static bool sniff(Bytes slice) {
        return slice.len() > 2 and slice[0] == 0xFF and slice[1] == Marker::SOI;
    }

    static Res<Decoder> init(Bytes slice) {
        if (not sniff(slice)) {
            logError("jpeg: not a JPEG image");
            return Error::invalidData("not a JPEG image");
        }

        Decoder dec{slice};
        try$(dec._loadFrame());
        return Ok(std::move(dec));
    }

    Bytes _slice;

    Decoder(Bytes slice)
        : _slice(slice) {}

    SegmentIter iterSegments() const {
        return {_slice};
    }

    Res<> _loadFrame() {
        for (auto segment : iterSegments()) {
            if (segment.marker == Marker::SOF0)
                return startOfFrame(segment.data);

            if (segment.marker.startsFrame()) {
                logError("jpeg: unsupported frame type {} (only baseline is supported)", segment.marker);
                return Error::notImplemented("unsupported jpeg frame type");
            }

            if (segment.marker == Marker::SOS or segment.marker == Marker::EOI)
                break;
        }

        logError("jpeg: missing start of frame");
        return Error::invalidData("missing start of frame");
    }

    // MARK: Quantization Tables -----------------------------------------------

    Array<Opt<Quant>, 4> _quant;

    Res<> defineQuantizationTable(Io::BScan s) {
        while (not s.ended()) {
            u8 infos = s.nextU8be();
            u8 id = infos & 0x0F;

            if (id > 3) {
                logError("jpeg: invalid quantization table id: {}", id);
                return Error::invalidData("invalid quantization table id");
            }

            Quant& quant = _quant[id].emplace();
            bool is16bit = (infos >> 4) == 1;

            for (usize i = 0; i < 64; ++i) {
                // NOTE: Quantization tables are stored in zig-zag order.
                quant[ZIGZAG[i]] = is16bit ? s.nextU16be() : s.nextU8be();
            }
        }

        return Ok();
    }

    // MARK: Start of frame ----------------------------------------------------

    struct Component {
        u8 id;
        u8 hSampling;
        u8 vSampling;
        u8 quantId;
        u8 dcHuffId = 0;
        u8 acHuffId = 0;

        usize blocksPerLine = 0;
        usize blocksPerColumn = 0;
        Box<Block[]> blocks = {};

        Block& blockAt(usize bx, usize by) {
            return blocks[by * blocksPerLine + bx];
        }

        Block const& blockAt(usize bx, usize by) const {
            return blocks[by * blocksPerLine + bx];
        }

        short sampleAt(usize px, usize py, u8 hMax, u8 vMax) const {
            usize sx = px * hSampling / hMax;
            usize sy = py * vSampling / vMax;
            return blockAt(sx / 8, sy / 8)[(sy % 8) * 8 + (sx % 8)];
        }
    };

    isize _width = 0;
    isize _height = 0;
    InlineVec<Component, 4> _components;
    u8 _hMax = 1;
    u8 _vMax = 1;
    usize _mcusPerLine = 0;
    usize _mcusPerColumn = 0;

    isize width() const { return _width; }

    isize height() const { return _height; }

    Opt<usize> _componentIndex(u8 id) const {
        for (usize i = 0; i < _components.len(); ++i) {
            if (_components[i].id == id)
                return Some(i);
        }
        return NONE;
    }

    // B.2.2 Frame header syntax
    // https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=39
    Res<> startOfFrame(Io::BScan s) {
        u8 precision = s.nextU8be();
        if (precision != 8) {
            logError("jpeg: invalid precision: {}", precision);
            return Error::invalidData("invalid precision");
        }

        _height = s.nextU16be();
        _width = s.nextU16be();

        if (_width == 0 or _height == 0) {
            logError("jpeg: invalid image size: {}x{}", _width, _height);
            return Error::invalidData("invalid image size");
        }

        u8 componentCount = s.nextU8be();
        if (componentCount != 1 and componentCount != 3) {
            logError("jpeg: invalid component count: {}", componentCount);
            return Error::invalidData("invalid component count");
        }

        for (u8 i = 0; i < componentCount; ++i) {
            u8 id = s.nextU8be();

            if (_componentIndex(id)) {
                logError("jpeg: duplicate component id: {}", id);
                return Error::invalidData("duplicate component id");
            }

            u8 sampling = s.nextU8be();
            u8 hSampling = sampling >> 4;
            u8 vSampling = sampling & 0xF;

            if (hSampling < 1 or hSampling > 4 or vSampling < 1 or vSampling > 4) {
                logError("jpeg: invalid sampling factors: {}x{}", hSampling, vSampling);
                return Error::invalidData("invalid sampling factors");
            }

            u8 quantId = s.nextU8be();
            if (quantId > 3) {
                logError("jpeg: invalid quantization table id: {}", quantId);
                return Error::invalidData("invalid quantization table id");
            }

            _components.pushBack(Component{
                .id = id,
                .hSampling = hSampling,
                .vSampling = vSampling,
                .quantId = quantId,
            });

            _hMax = max(_hMax, hSampling);
            _vMax = max(_vMax, vSampling);
        }

        _mcusPerLine = ceilDiv(_width, 8 * _hMax);
        _mcusPerColumn = ceilDiv(_height, 8 * _vMax);

        for (auto& c : _components) {
            c.blocksPerLine = _mcusPerLine * c.hSampling;
            c.blocksPerColumn = _mcusPerColumn * c.vSampling;
        }

        return Ok();
    }

    // MARK: Restart interval --------------------------------------------------

    usize _restartInterval = 0;

    Res<> defineRestartInterval(Io::BScan s) {
        _restartInterval = s.nextU16be();

        if (not s.ended()) {
            logError("jpeg: unexpected data after DRI marker");
            return Error::invalidData("unexpected data after DRI marker");
        }

        return Ok();
    }

    // MARK: Huffman Tables ----------------------------------------------------

    Array<Opt<Huff>, 4> _dcHuff;
    Array<Opt<Huff>, 4> _acHuff;

    Res<> defineHuffmanTable(Io::BScan s) {
        while (not s.ended()) {
            u8 infos = s.nextU8be();
            u8 id = infos & 0x0F;
            bool isAc = (infos >> 4) == 1;

            if (id > 3) {
                logError("jpeg: invalid huffman table id: {}", id);
                return Error::invalidData("invalid huffman table id");
            }

            auto& table = (isAc ? _acHuff : _dcHuff)[id].emplace();

            usize sum = 0;
            for (usize i = 1; i < 17; ++i) {
                sum += s.nextU8be();
                table.offs[i] = sum;
            }

            if (sum > 162) {
                logError("jpeg: invalid huffman table length: {}", sum);
                return Error::invalidData("invalid huffman table length");
            }

            for (usize i = 0; i < sum; ++i) {
                table.syms[i] = s.nextU8be();
            }
        }

        return Ok();
    }

    // MARK: Start of scan -----------------------------------------------------

    using Scan = InlineVec<usize, 4>;

    // B.2.3 Scan header syntax
    // https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=41
    Res<Scan> startOfScan(Io::BScan s) {
        u8 componentCount = s.nextU8be();
        if (componentCount == 0 or componentCount > _components.len()) {
            logError("jpeg: invalid scan component count: {}", componentCount);
            return Error::invalidData("invalid scan component count");
        }

        Scan scan;
        for (u8 i = 0; i < componentCount; ++i) {
            u8 id = s.nextU8be();

            auto index = _componentIndex(id);
            if (not index) {
                logError("jpeg: undefined component id: {}", id);
                return Error::invalidData("undefined component id");
            }

            u8 huffIds = s.nextU8be();
            auto& c = _components[*index];
            c.dcHuffId = huffIds >> 4;
            c.acHuffId = huffIds & 0xF;

            if (c.dcHuffId > 3 or not _dcHuff[c.dcHuffId]) {
                logError("jpeg: undefined dc huffman table id: {}", c.dcHuffId);
                return Error::invalidData("undefined dc huffman table id");
            }

            if (c.acHuffId > 3 or not _acHuff[c.acHuffId]) {
                logError("jpeg: undefined ac huffman table id: {}", c.acHuffId);
                return Error::invalidData("undefined ac huffman table id");
            }

            scan.pushBack(*index);
        }

        u8 ss = s.nextU8be();
        u8 se = s.nextU8be();
        u8 ahAl = s.nextU8be();

        if (ss != 0 or se != 63) {
            logError("jpeg: unexpected spectral selection");
            return Error::invalidData("unexpected spectral selection");
        }

        if (ahAl != 0) {
            logError("jpeg: unexpected successive approximation");
            return Error::invalidData("unexpected successive approximation");
        }

        if (not s.ended()) {
            logError("jpeg: unexpected data after SOS marker");
            return Error::invalidData("unexpected data after SOS marker");
        }

        return Ok(scan);
    }

    // MARK: Huffman Data ------------------------------------------------------

    Res<> decodeScan(Bytes entropyCoded, Scan const& scan) {
        Io::BScan s{entropyCoded};
        BitReader bs{s};
        Array<isize, 4> prevDc = {};
        usize mcuIndex = 0;

        auto nextMcu = [&] {
            if (_restartInterval and mcuIndex and mcuIndex % _restartInterval == 0) {
                bs.reset();
                prevDc = {};
            }
            mcuIndex++;
        };

        if (scan.len() == 1) {
            // A.2.2 Non-interleaved order
            // https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=29
            auto& c = _components[scan[0]];
            usize blocksPerLine = ceilDiv(ceilDiv(_width * c.hSampling, _hMax), 8);
            usize blocksPerColumn = ceilDiv(ceilDiv(_height * c.vSampling, _vMax), 8);

            for (usize by = 0; by < blocksPerColumn; ++by) {
                for (usize bx = 0; bx < blocksPerLine; ++bx) {
                    nextMcu();
                    try$(decodeBlock(bs, c, prevDc[0], c.blockAt(bx, by)));
                }
            }

            return Ok();
        }

        // A.2.3 Interleaved order
        // https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=30
        for (usize my = 0; my < _mcusPerColumn; ++my) {
            for (usize mx = 0; mx < _mcusPerLine; ++mx) {
                nextMcu();

                for (usize i = 0; i < scan.len(); ++i) {
                    auto& c = _components[scan[i]];

                    for (usize v = 0; v < c.vSampling; ++v) {
                        for (usize h = 0; h < c.hSampling; ++h) {
                            auto& block = c.blockAt(mx * c.hSampling + h, my * c.vSampling + v);
                            try$(decodeBlock(bs, c, prevDc[i], block));
                        }
                    }
                }
            }
        }

        return Ok();
    }

    Res<> decodeBlock(BitReader& bs, Component const& c, isize& prevDc, Block& block) {
        auto& dcHuff = *_dcHuff[c.dcHuffId];
        auto& acHuff = *_acHuff[c.acHuffId];

        // F.2.2.1 Huffman decoding of DC coefficients
        // https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=108
        u8 dcLen = try$(dcHuff.next(bs));
        if (dcLen > 11) {
            logError("jpeg: invalid dc huffman code length: {}", dcLen);
            return Error::invalidData("invalid dc huffman code length");
        }

        prevDc += extend(try$(bs.nextBits(dcLen)), dcLen);
        block[0] = prevDc;

        // F.2.2.2 Decoding procedure for AC coefficients
        // https://www.w3.org/Graphics/JPEG/itu-t81.pdf#page=109
        constexpr u8 END_OF_BLOCK = 0x00;
        usize k = 1;
        while (k < 64) {
            u8 sym = try$(acHuff.next(bs));

            if (sym == END_OF_BLOCK)
                break;

            u8 run = sym >> 4;
            u8 len = sym & 0xF;

            if (len > 10) {
                logError("jpeg: invalid ac huffman code length: {}", len);
                return Error::invalidData("invalid ac huffman code length");
            }

            k += run;
            if (k >= 64) {
                logError("jpeg: zero run length exceeds block size: {}", k);
                return Error::invalidData("zero run length exceeds block size");
            }

            block[ZIGZAG[k++]] = extend(try$(bs.nextBits(len)), len);
        }

        return Ok();
    }

    // MARK: Decoding ----------------------------------------------------------

    Res<> decode(Gfx::MutPixels pixels) {
        try$(_decodeScans());
        return _reconstruct(pixels);
    }

    Res<> _decodeScans() {
        for (auto& c : _components)
            c.blocks = Box<Block[]>::filled(c.blocksPerLine * c.blocksPerColumn);

        for (auto segment : iterSegments()) {
            auto marker = segment.marker;

            if (marker == Marker::DQT) {
                try$(defineQuantizationTable(segment.data));
            } else if (marker == Marker::DHT) {
                try$(defineHuffmanTable(segment.data));
            } else if (marker == Marker::DRI) {
                try$(defineRestartInterval(segment.data));
            } else if (marker == Marker::SOS) {
                auto scan = try$(startOfScan(segment.data));
                try$(decodeScan(segment.entropyCoded, scan));
            } else if (marker == Marker::EOI) {
                return Ok();
            } else if (marker == Marker::SOI or marker == Marker::SOF0) {
                continue;
            } else if (marker.application()) {
                logWarn("jpeg: skipping {}", marker);
            } else if (marker == Marker::COM) {
                logDebug("jpeg: skipping comment");
            } else if (marker == Marker::TEM) {
                logWarn("jpeg: ignoring TEM marker");
            } else {
                logWarn("jpeg: unknown marker: {}", marker);
            }
        }

        logError("jpeg: missing EOI marker");
        return Error::invalidData("missing EOI marker");
    }

    Res<> _reconstruct(Gfx::MutPixels pixels) {
        for (auto& c : _components) {
            if (not _quant[c.quantId]) {
                logError("jpeg: undefined quantization table id: {}", c.quantId);
                return Error::invalidData("undefined quantization table id");
            }

            auto& quant = *_quant[c.quantId];
            for (auto& block : c.blocks) {
                dequantize(block, quant);
                idct(block);
            }
        }

        for (isize y = 0; y < _height; ++y) {
            for (isize x = 0; x < _width; ++x) {
                pixels.storeUnsafe({x, y}, _colorAt(x, y));
            }
        }

        return Ok();
    }

    Gfx::Color _colorAt(usize x, usize y) const {
        auto sample = [&](usize i) -> f32 {
            return _components[i].sampleAt(x, y, _hMax, _vMax);
        };

        if (_components.len() == 1)
            return Gfx::yCbCrToRgb({sample(0), 0, 0});

        return Gfx::yCbCrToRgb({sample(0), sample(1), sample(2)});
    }

    // MARK: Dumping -----------------------------------------------------------

    void repr(Io::Emit& e) {
        e("JPEG image");
        e.indentNewline();
        e.ln("width: {}", width());
        e.ln("height: {}", height());
        e.ln("mcus: {}x{}", _mcusPerLine, _mcusPerColumn);

        e("quantization tables:");
        e.indentNewline();
        for (usize i = 0; i < _quant.len(); ++i) {
            if (_quant[i]) {
                e.ln("table {}:", i);
                e.indent();
                for (usize j = 0; j < 64; ++j) {
                    e("{:02} ", (*_quant[i])[j]);
                    if (j % 8 == 7)
                        e.newline();
                }
                e.newline();
                e.deindent();
            } else {
                e("table {}: none\n", i);
            }
        }
        e.deindent();

        e("components:");
        e.indentNewline();
        for (auto const& c : _components) {
            e("component {}:", c.id);
            e.indentNewline();
            e.ln("sampling: {}x{}", c.hSampling, c.vSampling);
            e.ln("quantId: {}", c.quantId);
            e.ln("dcHuffId: {}", c.dcHuffId);
            e.ln("acHuffId: {}", c.acHuffId);
            e.ln("blocks: {}x{}", c.blocksPerLine, c.blocksPerColumn);
            e.deindent();
        }
        e.deindent();

        e("restart interval: {}\n", _restartInterval);

        e("huffman tables:");
        e.indentNewline();

        for (usize i = 0; i < 4; i++) {
            if (_dcHuff[i]) {
                auto table = (*_dcHuff[i]);
                e("DC table {}:\n", i);
                e.indent();
                for (usize j = 0; j < 16; ++j) {
                    e("{:02x}:", table.offs[j]);
                    for (usize k = table.offs[j]; k < table.offs[j + 1]; ++k) {
                        e(" {:02x}", table.syms[k]);
                    }
                    e.newline();
                }
                e.deindent();
            } else {
                e("DC table {}: none\n", i);
            }
        }

        for (usize i = 0; i < 4; i++) {
            if (_acHuff[i]) {
                auto table = (*_acHuff[i]);
                e.ln("AC table {}:", i);
                e.indent();
                for (usize j = 0; j < 16; ++j) {
                    e("{:02x}:", table.offs[j]);
                    for (usize k = table.offs[j]; k < table.offs[j + 1]; ++k) {
                        e(" {:02x}", table.syms[k]);
                    }
                    e.newline();
                }
                e.deindent();
            } else {
                e.ln("AC table {}: none", i);
            }
        }

        e.deindent();
        e.deindent();
    }
};

} // namespace Karm::Jpeg
