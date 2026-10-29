#include <karm/entry>

import Karm.Core;
import Karm.Icu;

using namespace Karm;

struct TextCell {
    Rune rune = '\0';
    Icu::Script script = Icu::Script::COMMON;
    Icu::GraphemeBoundary graphemeBoundary = Icu::GraphemeBoundary::NO;
    Icu::LineBoundary lineBoundary = Icu::LineBoundary::ALLOWED;

    void repr(Io::Emit& e) const {
        e("\n(cell '{:#c}' {:#08x} script:{} graphemeBoundary:{} lineBoundary:{})", rune, rune, script, graphemeBoundary, lineBoundary);
    }
};

Async::Task<> entryPointAsync(Sys::Env&, Async::CancellationToken) {
    Str text = "Grandma Zoë said “תודה רבה” 3 times and “شكرًا” twice to the café baker, shared 鯛焼き with her neighbor प्रिया, and the whole family 👨‍👩‍👧 waved 👋🏽 goodbye 🇯🇵.";
    Vec<TextCell> cells;
    Icu::GraphemeSegmenter segmenter = {};
    Icu::LineTailoring tailoring{.words = Icu::WordBreaking::NORMAL};
    Icu::LineSegmenter lineSegmenter = {tailoring};

    for (auto rune : iterRunes(text)) {
        cells.pushBack({
            rune,
            Icu::Properties::of(rune).script(),
            segmenter.advance(rune),
            lineSegmenter.advance(rune).current,
        });
    }

    Icu::Script previousScript = Icu::Script::LATIN;
    for (auto& cell : cells) {
        auto currentScript = cell.script;
        if (currentScript == Icu::Script::INHERITED)
            currentScript = previousScript;

        // FIXME: This is not correct in all cases (see: https://www.unicode.org/reports/tr24/#Common)
        //        Maybe implement https://github.com/ImageMagick/pango/blob/main/pango/pango-script.c
        if (currentScript == Icu::Script::COMMON)
            currentScript = previousScript;

        cell.script = currentScript;
        previousScript = currentScript;
    }

    Sys::println("result: {}", cells);
    co_return Ok();
}
