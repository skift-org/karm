#include <karm/entry>

import Karm.Cli;
import Karm.Sys;
import Karm.Gfx;
import Karm.Math;

using namespace Karm;

void report(Vec<Duration>& samples) {
    sort(samples, [](auto& a, auto& b) {
        return a.toUSecs() <=> b.toUSecs();
    });

    f64 sum = 0;
    for (auto& s : samples)
        sum += s.toUSecs();

    Sys::println("");
    Sys::println("median: {}", samples[samples.len() / 2]);
    Sys::println("average: {}", Duration::fromUSecs(sum / samples.len()));
    Sys::println("min: {}", first(samples));
    Sys::println("max: {}", last(samples));
}

Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    Vec<Duration> samples;
    auto image = Gfx::Image::alloc({1000, 1000});

    Sys::println("strokes:");
    for (isize i = 0; i < 100; i++) {
        auto start = Sys::now();
        for (isize size = 100; size < 1000; size += 10) {
            f64 scale = size / 100.0;

            Gfx::CpuCanvas g;
            g.begin(image->mutPixels());
            g.scale(scale);

            for (isize i = 0; i < 50; i++) {
                Math::Rand rand{};

                f64 s = rand.nextInt(4, 10);
                s *= s;

                g.beginPath();
                g.ellipse({
                    rand.nextVec2(Math::Recti{100, 100}).cast<f64>(),
                    s,
                });

                g.strokeStyle(
                    Gfx::stroke(Gfx::randomColor(rand))
                        .withWidth(rand.nextInt(2, s))
                );
                g.stroke();
            }
        }

        auto elapsed = Sys::now() - start;
        samples.pushBack(elapsed);

        Sys::print("sampling {}/100: {}\r", i + 1, elapsed);
    }

    report(samples);

    Sys::println("");
    Sys::println("text:");
    samples.clear();

    auto font = Gfx::Font::fallback();
    Gfx::CpuCanvas g;

    for (isize i = 0; i < 100; i++) {
        auto start = Sys::now();

        g.begin(image->mutPixels());
        g.fillStyle(Gfx::WHITE);

        for (isize line = 0; line < 100; line++) {
            Math::Vec2f baseline = {8.25 * (line % 4), 16.0 + (line % 60) * 16.0};
            for (auto rune : iterRunes(Str{"the quick brown fox jumps over the lazy dog"})) {
                auto glyph = font.glyph(rune);
                g.fill(font, glyph, baseline);
                baseline.x += font.advance(glyph);
            }
        }

        g.end();

        auto elapsed = Sys::now() - start;
        samples.pushBack(elapsed);

        Sys::print("sampling {}/100: {}\r", i + 1, elapsed);
    }

    report(samples);

    co_return Ok();
}
