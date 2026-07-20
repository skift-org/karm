#include <karm/test>

import Karm.Core;
import Karm.Gfx;
import Karm.Math;
import Karm.Print;

namespace Karm::Print::Tests {

test$("pdf-printer-generate") {
    PdfPrinter printer;

    auto& canvas = printer.beginPage({800, 600});

    canvas.beginPath();
    canvas.moveTo({100, 100}, {});
    canvas.lineTo({700, 100}, {});
    canvas.lineTo({700, 500}, {});
    canvas.lineTo({100, 500}, {});
    canvas.closePath();
    canvas.fillStyle(Gfx::RED);
    canvas.fill(Gfx::FillRule::NONZERO);

    auto surface = Gfx::Image::alloc({64, 64});
    auto pixels = surface->mutPixels();

    for (isize y = 0; y < pixels.height(); y++) {
        for (isize x = 0; x < pixels.width(); x++) {
            pixels.loadUnsafe({x, y}) = Gfx::GREEN;
        }
    }

    canvas.blit({0, 0, 64, 64}, {200, 200, 128, 128}, surface);

    Io::BufferWriter bw;
    try$(printer.write(bw));

    auto bytes = bw.bytes();
    expectEq$(sub(bytes, 0, 8), Karm::bytes(Str{"%PDF-2.0"}));

    return Ok();
}

} // namespace Karm::Print::Tests
