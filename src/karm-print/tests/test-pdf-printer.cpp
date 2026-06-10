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

    Array<u8, 64 * 64 * 4> px;
    for (usize i = 0; i < px.len(); i++)
        px[i] = i;
    Gfx::Pixels pixels{px.buf(), {64, 64}, 64 * 4, Gfx::RGBA8888};
    canvas.blit({0, 0, 64, 64}, {200, 200, 128, 128}, pixels);

    Io::BufferWriter bw;
    try$(printer.write(bw));

    auto bytes = bw.bytes();
    expectEq$(sub(bytes, 0, 8), Karm::bytes(Str{"%PDF-2.0"}));

    return Ok();
}

} // namespace Karm::Print::Tests
