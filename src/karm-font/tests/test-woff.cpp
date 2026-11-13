#include <karm/test>

import Karm.Core;
import Karm.Font;
import Karm.Font.Woff;
import Karm.Gfx;
import Karm.Ref;
import Karm.Sys;

// The fonts in res/woff hold a handful of glyphs picked to go through the
// decoders: an empty one, simple ones with curves, instructions, far away
// points, long runs of similar points and a bounding box that isn't the one
// of its points, and composites with and without instructions.
//
//   tiny.woff2   written by the reference implementation (google/woff2 1.0.2)
//   tiny.ttf     what the reference implementation decodes tiny.woff2 to
//   tiny.woff    tiny.ttf wrapped table by table, in the order of the file
//   hmtx.woff2   written by fontTools 4.68, with the 'hmtx' transform on
//   hmtx.ttf     what the reference implementation decodes hmtx.woff2 to

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Font::Woff::Tests {

static Res<Vec<u8>> _read(Str name) {
    return Sys::readAll("bundle://karm-font.tests/woff"_url / name);
}

test$("woff1-decode") {
    auto woff = try$(_read("tiny.woff"));
    auto ttf = try$(_read("tiny.ttf"));

    assert$(sniffWoff1(woff));
    assertNot$(sniffWoff2(woff));

    auto decoded = try$(decodeWoff1(woff));
    assertEq$(decoded.len(), ttf.len());
    assert$(decoded == ttf);

    return Ok();
}

test$("woff2-decode") {
    auto woff2 = try$(_read("tiny.woff2"));
    auto ttf = try$(_read("tiny.ttf"));

    assert$(sniffWoff2(woff2));
    assertNot$(sniffWoff1(woff2));

    auto decoded = try$(decodeWoff2(woff2));
    assertEq$(decoded.len(), ttf.len());
    assert$(decoded == ttf);

    return Ok();
}

test$("woff2-decode-hmtx-transform") {
    auto woff2 = try$(_read("hmtx.woff2"));
    auto ttf = try$(_read("hmtx.ttf"));

    auto decoded = try$(decodeWoff2(woff2));
    assertEq$(decoded.len(), ttf.len());
    assert$(decoded == ttf);

    return Ok();
}

test$("woff-decode-truncated") {
    auto woff = try$(_read("tiny.woff"));
    for (usize i = 0; i < woff.len(); i++)
        assertNot$(decodeWoff1(sub(woff, 0, i)).has());

    auto woff2 = try$(_read("tiny.woff2"));
    for (usize i = 0; i < woff2.len(); i++)
        assertNot$(decodeWoff2(sub(woff2, 0, i)).has());

    return Ok();
}

// A broken file may or may not decode to something, what matters is that
// the decoders make it to the end.
test$("woff-decode-corrupt") {
    auto corrupt = [](Bytes data, usize end, auto decode) {
        for (usize i = 0; i < end; i++) {
            for (u8 mask : {0x01, 0x80, 0xff}) {
                Vec<u8> copy = data;
                copy[i] ^= mask;
                (void)decode(copy);
            }
        }
    };

    // NOTE: Only the header and the directory, inflate doesn't cope with
    //       corrupt streams yet.
    auto woff = try$(_read("tiny.woff"));
    corrupt(woff, 44 + 20 * 10, decodeWoff1);

    for (auto name : {"tiny.woff2"s, "hmtx.woff2"s}) {
        auto woff2 = try$(_read(name));
        corrupt(woff2, woff2.len(), decodeWoff2);
    }

    return Ok();
}

test$("woff-load-fontface") {
    auto ttf = try$(Font::loadFontface("bundle://karm-font.tests/woff/tiny.ttf"_url));

    for (auto name : {"tiny.woff"s, "tiny.woff2"s}) {
        auto face = try$(Font::loadFontface("bundle://karm-font.tests/woff"_url / name));

        assertEq$(face->attrs().family, "Karm Woff Test"_sym);
        for (Rune r : {U'A', U' ', U's', U'w', U'\u00c1', U'\u00c5'}) {
            assertNe$(face->glyph(r).index, 0);
            assertEq$(face->glyph(r).index, ttf->glyph(r).index);
            assertEq$(face->advance(face->glyph(r)), ttf->advance(ttf->glyph(r)));
        }
    }

    auto face = try$(Font::loadFontface(try$(_read("tiny.woff2"))));
    assertEq$(face->attrs().family, "Karm Woff Test"_sym);

    return Ok();
}

} // namespace Karm::Font::Woff::Tests
