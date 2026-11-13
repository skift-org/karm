module;

#include <karm/macros>

export module Karm.Qoi:decoder;

import Karm.Core;
import Karm.Gfx.Pixels;

import :base;

namespace Karm::Qoi {

export struct Decoder : Io::BChunk {
    using Width = Io::BField<i32be, 4>;
    using Height = Io::BField<i32be, 8>;
    using Channels = Io::BField<u8be, 12>;
    using ColorSpace = Io::BField<u8be, 13>;

    isize _width = 0;
    isize _height = 0;
    u8 _channels = 0;
    u8 _colorSpace = 0;

    Res<Bytes> magic() const { return begin().nextBytes(4); }

    isize width() const { return _width; }

    isize height() const { return _height; }

    u8 channels() const { return _channels; }

    u8 colorSpace() const { return _colorSpace; }

    Decoder(Bytes slice) : BChunk(slice) {}

    static bool sniff(Bytes slice) {
        return slice.len() >= 4 and slice[0] == 0x71 and slice[1] == 0x6F and
               slice[2] == 0x69 and slice[3] == 0x66;
    }

    static Res<Decoder> init(Bytes slice) {
        if (slice.len() < 14) {
            return Error::invalidData("image too small");
        }

        auto dec = Decoder(slice);
        if (try$(dec.magic()) != MAGIC) {
            return Error::invalidData("invalid magic");
        }

        dec._width = try$(dec.get<Width>());
        dec._height = try$(dec.get<Height>());
        dec._channels = try$(dec.get<Channels>());
        dec._colorSpace = try$(dec.get<ColorSpace>());

        if (dec._width < 0 or dec._height < 0) {
            return Error::invalidData("invalid image size");
        }

        if (not(dec.channels() == 4 or dec.channels() == 3)) {
            return Error::invalidData("invalid number of channels");
        }

        if (not(dec.colorSpace() == 0 or dec.colorSpace() == 1)) {
            return Error::invalidData("invalid color space");
        }

        return Ok(dec);
    }

    Res<> decode(Gfx::MutPixels dest) {
        usize run = 0;
        Array<Gfx::Color, 64> index{};
        Gfx::Color pixel = Gfx::BLACK;

        auto s = try$(begin(14));

        for (isize y = 0; y < height(); y++) {
            for (isize x = 0; x < width(); x++) {
                if (s.ended()) {
                    return Error::invalidData("unexpected end of file");
                }

                if (run > 0) {
                    run--;
                    dest.store({x, y}, pixel);
                    continue;
                }

                auto b1 = try$(s.nextU8be());
                if (b1 == Chunk::RGB) {
                    pixel.red = try$(s.nextU8be());
                    pixel.green = try$(s.nextU8be());
                    pixel.blue = try$(s.nextU8be());
                } else if (b1 == Chunk::RGBA) {
                    pixel.red = try$(s.nextU8be());
                    pixel.green = try$(s.nextU8be());
                    pixel.blue = try$(s.nextU8be());
                    pixel.alpha = try$(s.nextU8be());
                } else if ((b1 & Chunk::MASK) == Chunk::INDEX) {
                    pixel = index[b1];
                } else if ((b1 & Chunk::MASK) == Chunk::DIFF) {
                    pixel.red += ((b1 >> 4) & 0x03) - 2;
                    pixel.green += ((b1 >> 2) & 0x03) - 2;
                    pixel.blue += (b1 & 0x03) - 2;
                } else if ((b1 & Chunk::MASK) == Chunk::LUMA) {
                    auto b2 = try$(s.nextU8be());
                    auto vg = (b1 & 0x3f) - 32;
                    pixel.red += vg - 8 + ((b2 >> 4) & 0x0f);
                    pixel.green += vg;
                    pixel.blue += vg - 8 + (b2 & 0x0f);
                } else if ((b1 & Chunk::MASK) == Chunk::RUN) {
                    run = b1 & (~Chunk::MASK);
                } else {
                    return Error::invalidData("invalid chunk");
                }

                index[hashColor(pixel) % index.len()] = pixel;
                dest.store({x, y}, pixel);
            }
        }

        if (try$(s.nextBytes(8)) != END) {
            return Error::invalidData("missing end marker");
        }

        return Ok();
    }
};

} // namespace Karm::Qoi
