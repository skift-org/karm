#include <karm/test>

import Karm.Core;
import Karm.Gfx;
import Karm.Math;

using namespace Karm::Literals;

namespace Karm::Gfx::Tests {

static void _drawPrimitives(Canvas& g) {
    auto font = Font::fallback();

    g.clear(Gfx::BLACK);
    g.fillStyle(Gfx::WHITE);

    g.beginPath();
    g.rect({2, 2, 24, 12});
    g.rect({5, 5, 18, 6});
    g.fill(FillRule::EVENODD);

    g.fill(Math::Rectf{30.5, 2.25, 12, 10}, 3);
    g.fill(Math::Recti{2, 18, 10, 10});
    g.fill(font, font.glyph('A'), {30, 28});
}

static bool _hasOp(Str disasm, Str op) {
    for (auto line : iterSplit(disasm, '\n'))
        if (startWith(line, op) == Match::PARTIAL and line.len() > op.len() and line[op.len()] == ' ')
            return true;
    return false;
}

test$("snapshot-records-primitives") {
    Snapshot::Recorder recorder{{48, 32}};
    _drawPrimitives(recorder);
    auto disasm = recorder.finalize().disasm();

    assert$(_hasOp(disasm, "RECT"s));
    assert$(_hasOp(disasm, "FILL_RECT"s));
    assert$(_hasOp(disasm, "FILL_GLYPH"s));
    assert$(not _hasOp(disasm, "MOVE_TO"s));
    assert$(not _hasOp(disasm, "LINE_TO"s));

    return Ok();
}

test$("snapshot-replays-primitives-like-direct") {
    auto direct = Image::alloc({48, 32}, Gfx::RGBA8888);
    CpuCanvas g;
    g.begin(direct->mutPixels());
    _drawPrimitives(g);
    g.end();

    Snapshot::Recorder recorder{{48, 32}};
    _drawPrimitives(recorder);
    auto replayed = try$(recorder.finalize().rasterize());

    for (isize y = 0; y < 32; y++)
        for (isize x = 0; x < 48; x++)
            assertEq$(replayed->pixels().loadUnsafe({x, y}), direct->pixels().loadUnsafe({x, y}));

    return Ok();
}

} // namespace Karm::Gfx::Tests
