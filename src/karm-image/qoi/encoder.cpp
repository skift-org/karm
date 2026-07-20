module;

#include <karm/macros>

export module Karm.Image:qoi.encoder;

import Karm.Core;
import Karm.Gfx.Pixels;

import :qoi.base;

namespace Karm::Image::Qoi {

export Res<> encode(Gfx::Pixels pixels, Io::BEmit& e) {
    try$(e.writeBytes(MAGIC));
    try$(e.writeU32be(pixels.width()));
    try$(e.writeU32be(pixels.height()));
    try$(e.writeU8be(4)); // Channels
    try$(e.writeU8be(1)); // Color space

    Array<Gfx::Color, 64> index = {};
    Gfx::Color curr = Gfx::BLACK;
    Gfx::Color prev = Gfx::BLACK;

    isize run = 0;

    for (isize y = 0; y < pixels.height(); y++) {
        for (isize x = 0; x < pixels.width(); x++) {
            prev = curr;
            curr = pixels.loadUnsafe({x, y});
            bool end = x == pixels.width() - 1 and
                       y == pixels.height() - 1;

            if (curr == prev) {
                run++;
                if (run == 62 or end) {
                    try$(e.writeU8be(Chunk::RUN | (run - 1)));
                    run = 0;
                }
                continue;
            }

            if (run > 0) {
                try$(e.writeU8be(Chunk::RUN | (run - 1)));
                run = 0;
            }

            usize index_pos = hashColor(curr) % 64;

            if (index[index_pos] == curr) {
                try$(e.writeU8be(Chunk::INDEX | index_pos));
                continue;
            }

            index[index_pos] = curr;

            if (curr.alpha == prev.alpha) {
                i8 vr = curr.red - prev.red;
                i8 vg = curr.green - prev.green;
                i8 vb = curr.blue - prev.blue;

                i8 vg_r = vr - vg;
                i8 vg_b = vb - vg;

                if (
                    vr > -3 and vr < 2 and
                    vg > -3 and vg < 2 and
                    vb > -3 and vb < 2
                ) {
                    try$(e.writeU8be(Chunk::DIFF | (vr + 2) << 4 | (vg + 2) << 2 | (vb + 2)));
                    continue;
                }

                if (
                    vg_r > -9 and vg_r < 8 &&
                    vg > -33 and vg < 32 &&
                    vg_b > -9 and vg_b < 8
                ) {
                    try$(e.writeU8be(Chunk::LUMA | (vg + 32)));
                    try$(e.writeU8be((vg_r + 8) << 4 | (vg_b + 8)));
                } else {
                    try$(e.writeU8be(Chunk::RGB));
                    try$(e.writeU8be(curr.red));
                    try$(e.writeU8be(curr.green));
                    try$(e.writeU8be(curr.blue));
                }
                continue;
            }

            try$(e.writeU8be(Chunk::RGBA));
            try$(e.writeU8be(curr.red));
            try$(e.writeU8be(curr.green));
            try$(e.writeU8be(curr.blue));
            try$(e.writeU8be(curr.alpha));
        }
    }

    try$(e.writeBytes(END));

    return Ok();
}

} // namespace Karm::Image::Qoi
