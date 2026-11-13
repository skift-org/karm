#include <karm/entry>

import Karm.Font.Ttf;
import Karm.Font.Woff;
import Karm.Font;
import Karm.Gfx;
import Karm.Image;
import Karm.Math;
import Karm.Sys;

using namespace Karm;

static Res<> _dumpGpos(Font::Ttf::Gpos const& gpos) {
    Sys::println("GPOS table:");
    if (not gpos.present()) {
        Sys::println("  not present");
        return Ok();
    }

    auto scriptList = try$(gpos.scriptList());
    auto scripts = try$(scriptList.records());
    Sys::println("  ScriptList (len:{})", scripts.len());
    for (auto const& scriptRecord : scripts) {
        auto script = try$(scriptList.script(scriptRecord));
        Sys::println("    '{}'", script.tag);

        Sys::println("      '{}'", try$(script.defaultLangSys()).tag);
        for (auto const& langRecord : try$(script.records())) {
            auto lang = try$(script.langSys(langRecord));
            Sys::println("      '{}'", lang.tag);

            Sys::println("        Features:");
            for (u16 feat : try$(lang.featureIndices())) {
                Sys::println("          '{}'", feat);
            }
        }
    }

    auto featureList = try$(gpos.featureList());
    auto features = try$(featureList.records());
    Sys::println("  FeatureList (len:{})", features.len());
    for (auto const& featureRecord : features) {
        auto feat = try$(featureList.feature(featureRecord));
        Sys::println("    '{}'", feat.tag);
        for (u16 lookup : try$(feat.lookupIndices())) {
            Sys::println("      LookupIndex: {}", lookup);
        }
    }

    Sys::println("  LookupList (len:{})", try$(try$(gpos.lookupList()).offsets()).len());
    return Ok();
}

static Res<> _dumpName(Font::Ttf::Name const& name) {
    Sys::println("Name table:");
    if (not name.present()) {
        Sys::println("  not present");
        return Ok();
    }

    Sys::println("  Records");
    for (auto const& record : try$(name.records())) {
        if (not record.isUnicode())
            continue;
        Sys::println("    {}:{}:{} {}={:#}", (u16)record.platformId, (u16)record.encodingId, (u16)record.languageId, (u16)record.nameId, name.string(record));
    }
    return Ok();
}

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    if (env.argsLen() < 1)
        co_return Error::invalidInput("Usage: ttf-dump <verb> <args...>");

    auto verb = env[0];

    if (verb == "dump-ttf") {
        if (env.argsLen() != 2)
            co_return Error::invalidInput("Usage: ttf-dump dump-ttf <url>");

        auto url = Ref::parseUrlOrPath(env[1], env.cwd());
        auto file = co_try$(Sys::File::open(url));
        auto map = co_try$(Sys::mmap(file));
        auto ttf = co_try$(Font::Ttf::Parser::init(map.bytes()));

        Sys::println("ttf is valid");
        co_try$(_dumpGpos(ttf._gpos));
        co_try$(_dumpName(ttf._name));

        co_return Ok();
    } else if (verb == "dump-db") {
        Font::Database db;
        co_try$(db.loadSystemFonts());
        co_return Ok();
    } else if (verb == "dump-attr") {
        if (env.argsLen() != 2)
            co_return Error::invalidInput("Usage: ttf-dump dump-attr <url>");

        auto url = Ref::parseUrlOrPath(env[1], env.cwd());
        auto font = co_try$(Font::loadFontface(url));

        Sys::println("{}", font->attrs());
        co_return Ok();
    } else if (verb == "decode-woff") {
        if (env.argsLen() != 3)
            co_return Error::invalidInput("Usage: ttf-dump decode-woff <url> <output>");

        auto url = Ref::parseUrlOrPath(env[1], env.cwd());
        auto file = co_try$(Sys::File::open(url));
        auto map = co_try$(Sys::mmap(file));

        Vec<u8> sfnt;
        if (Font::Woff::sniffWoff1(map.bytes()))
            sfnt = co_try$(Font::Woff::decodeWoff1(map.bytes()));
        else if (Font::Woff::sniffWoff2(map.bytes()))
            sfnt = co_try$(Font::Woff::decodeWoff2(map.bytes()));
        else
            co_return Error::invalidInput("not a woff or woff2 file");

        co_try$(Sys::writeAll(Ref::parseUrlOrPath(env[2], env.cwd()), sfnt));
        co_return Ok();
    } else if (verb == "render") {
        if (env.argsLen() < 4)
            co_return Error::invalidInput("Usage: ttf-dump render <url> <output.bmp> <line...>");

        static constexpr f64 FONT_SIZE = 64;
        static constexpr f64 LINE_HEIGHT = FONT_SIZE * 1.4;
        static constexpr f64 PADDING = 24;

        auto url = Ref::parseUrlOrPath(env[1], env.cwd());
        Gfx::Font font{co_try$(Font::loadFontface(url)), FONT_SIZE};

        struct Placed {
            Gfx::Glyph glyph;
            Math::Vec2f baseline;
        };

        Vec<Placed> placed;
        f64 width = 0;
        usize lines = env.argsLen() - 3;
        for (usize i = 0; i < lines; i++) {
            Math::Vec2f baseline = {PADDING, PADDING + FONT_SIZE + i * LINE_HEIGHT};
            Opt<Gfx::Glyph> prev = NONE;
            for (auto rune : iterRunes(env[3 + i])) {
                auto glyph = font.glyph(rune);
                if (prev)
                    baseline.x += font.kern(*prev, glyph);
                placed.pushBack({glyph, baseline});
                baseline.x += font.advance(glyph);
                prev = Some(glyph);
            }
            width = max(width, baseline.x);
        }

        auto surface = Gfx::Image::alloc(
            Math::Vec2f{width + PADDING, lines * LINE_HEIGHT + PADDING * 2}.ceil().cast<isize>(),
            Gfx::RGBA8888
        );
        surface->mutPixels().clear(Gfx::WHITE);

        Gfx::CpuCanvas g;
        g.begin(surface->mutPixels());
        g.fillStyle(Gfx::BLACK);
        for (auto& p : placed)
            g.fill(font, p.glyph, p.baseline);
        g.end();

        co_try$(Image::save(surface->pixels(), Ref::parseUrlOrPath(env[2], env.cwd())));
        co_return Ok();
    } else {
        Sys::errln("unknown verb: {} (expected: dump-ttf, dump-db, dump-attr, decode-woff, render)", verb);
        co_return Error::invalidInput();
    }
}
