
module;

#include <karm/macros>

export module Karm.Font.Ttf:fontface;

import Karm.Core;
import Karm.Sys;
import Karm.Gfx;
import Karm.Image;
import Karm.Logger;

import :parser;
import :subset;

namespace Karm::Font::Ttf {

export struct Fontface : Gfx::Fontface {
    Union<Sys::Mmap, Vec<u8>> _data;
    Parser _parser;
    mutable Map<Rune, Gfx::Glyph> _cachedEntries;
    mutable Map<Gfx::Glyph, f64> _cachedAdvances;
    mutable Map<Pair<Gfx::Glyph>, f64> _cachedKerns;
    mutable Opt<Gfx::FontMetrics> _cachedMetrics;
    mutable Map<u16, Opt<Rc<Gfx::Image>>> _cachedBitmaps;
    f64 _unitPerEm = 0;

    static Res<Rc<Fontface>> load(Sys::Mmap&& mmap) {
        auto ttf = try$(Ttf::Parser::init(mmap.bytes()));
        return Ok(makeRc<Fontface>(std::move(mmap), ttf));
    }

    static Res<Rc<Fontface>> load(Vec<u8>&& buf) {
        // NOTE: The parser points into the buffer, which is fine as moving
        //       a Vec doesn't move what it holds.
        auto ttf = try$(Ttf::Parser::init(buf));
        return Ok(makeRc<Fontface>(std::move(buf), ttf));
    }

    Fontface(Union<Sys::Mmap, Vec<u8>> data, Parser parser)
        : _data(std::move(data)),
          _parser(std::move(parser)) {
        _unitPerEm = _parser.unitPerEm();
    }

    Bytes bytes() const {
        return _data.visit(
            [](Sys::Mmap const& mmap) -> Bytes {
                return mmap.bytes();
            },
            [](Vec<u8> const& buf) -> Bytes {
                return buf;
            }
        );
    }

    Gfx::FontMetrics metrics() const override {
        if (_cachedMetrics.has())
            return _cachedMetrics.expect();

        auto m = _parser.metrics();

        auto glyphTop = [&](Rune rune) -> Opt<f64> {
            auto g = glyph(rune);
            if (g.index == 0)
                return NONE;
            f64 top = -_parser.glyphMetrics(g).unwrapOr({}).y;
            if (top <= 0)
                return NONE;
            return Some(top);
        };

        Opt<f64> xHeight = NONE;
        Opt<f64> capHeight = NONE;

        if (_parser._os2.present()) {
            if (auto h = _parser._os2.xHeight(); h and *h > 0)
                xHeight = Some<f64>(*h);
            if (auto h = _parser._os2.capHeight(); h and *h > 0)
                capHeight = Some<f64>(*h);
        }

        if (not xHeight)
            xHeight = glyphTop('x');
        if (not capHeight)
            capHeight = glyphTop('H');

        // https://drafts.csswg.org/css-values/#ex and #cap fallbacks: 0.5em and the ascent.
        _cachedMetrics.emplace(Gfx::FontMetrics{
            .ascend = m.ascend / _unitPerEm,
            .captop = capHeight.unwrapOr(m.ascend) / _unitPerEm,
            .descend = m.descend / _unitPerEm,
            .linegap = m.linegap / _unitPerEm,
            .advance = 0,
            .xHeight = xHeight.unwrapOr(_unitPerEm / 2) / _unitPerEm,
        });
        return _cachedMetrics.expect();
    }

    Gfx::FontAttrs attrs() const override {
        Gfx::FontAttrs attrs;

        if (_parser._name.present()) {
            auto name = _parser._name;
            attrs.family = Symbol::from(name.lookupString(Ttf::Name::FAMILY));
        }

        if (_parser._post.present()) {
            if (_parser._post.isFixedPitch())
                attrs.monospace = Gfx::Monospace::YES;

            if (_parser._post.italicAngle() != 0)
                attrs.style = Gfx::FontStyle::ITALIC;
        }

        if (_parser._os2.present()) {
            attrs.weight = Gfx::FontWeight{_parser._os2.weightClass()};
            attrs.stretch = Gfx::FontStretch{static_cast<u16>(_parser._os2.widthClass() * 100)};
        }

        return attrs;
    }

    Gfx::Run shape(Slice<Rune> runes) const override {
        Gfx::Run run = {};
        Gfx::Glyph prev = Gfx::Glyph::TOFU;
        for (auto [r, i] : iter(runes) | Index()) {
            auto g = glyph(r);
            run.items.pushBack({
                .glyph = g,
                .advance = advance(g) + kern(prev, g),
                .runeIndex = i,
            });
            prev = g;
        }
        return run;
    }

    Res<Gfx::FontSubset> subset(Slice<Gfx::Glyph> glyphs) const override {
        return Ttf::subset(_parser, glyphs);
    }

    Gfx::Glyph glyph(Rune rune) const override {
        auto glyph = _cachedEntries.lookup(rune);
        if (glyph.has())
            return glyph.expect();
        auto g = _parser.glyph(rune).unwrapOr(Gfx::Glyph(0));
        _cachedEntries.put(rune, g);
        return g;
    }

    f64 advance(Gfx::Glyph glyph) const override {
        auto advance = _cachedAdvances.lookup(glyph);
        if (advance.has())
            return advance.expect();
        auto a = _parser.glyphMetrics(glyph).unwrapOr({}).advance / _unitPerEm;
        _cachedAdvances.put(glyph, a);
        return a;
    }

    f64 kern(Gfx::Glyph prev, Gfx::Glyph curr) const override {
        auto kern = _cachedKerns.lookup(Tuple{prev, curr});
        if (kern.has())
            return kern.expect();

        auto k = _parser.glyphKern(prev, curr) / _unitPerEm;
        _cachedKerns.put(Tuple{prev, curr}, k);
        return k;
    }

    void glyphContour(Gfx::Canvas& g, Gfx::Glyph glyph) const override {
        g.scale(1.0 / _unitPerEm);
        if (auto res = _parser.glyphContour(g, glyph); not res)
            logWarn("ttf: malformed outline for glyph {}: {}", glyph.index, res.none());
    }

    Opt<Rc<Gfx::Image>> glyphBitmap(Gfx::Glyph glyph) const {
        if (auto cached = _cachedBitmaps.lookup(glyph.index))
            return cached.expect();

        Opt<Rc<Gfx::Image>> image = NONE;
        if (auto bitmap = _parser.glyphBitmap(glyph).unwrapOr(NONE)) {
            if (auto decoded = Image::load(bitmap->png))
                image = Some(decoded.take());
            else
                logWarn("ttf: failed to decode bitmap for glyph {}", glyph.index);
        }

        _cachedBitmaps.put(glyph.index, image);
        return image;
    }

    Flags<Gfx::GlyphAttr> glyphAttr(Gfx::Glyph glyph) const override {
        Flags<Gfx::GlyphAttr> attrs = NONE;
        if (_parser.glyphHasColor(glyph).unwrapOr(false) or glyphBitmap(glyph).has())
            attrs.set(Gfx::GlyphAttr::COLORED);
        return attrs;
    }

    void paintGlyph(Gfx::Canvas& g, Gfx::Glyph glyph) const override {
        if (_parser.glyphHasColor(glyph).unwrapOr(false)) {
            g.scale(1.0 / _unitPerEm);
            if (auto res = _parser.glyphPaint(g, glyph); not res)
                logWarn("ttf: malformed color glyph {}: {}", glyph.index, res.none());
            return;
        }

        if (auto image = glyphBitmap(glyph)) {
            // NOTE: Bitmap metrics are in pixels of the strike, one em is ppem pixels.
            auto bitmap = _parser.glyphBitmap(glyph).unwrapOr(NONE).expect();
            f64 ppem = bitmap.ppem;
            Math::Rectf bound = {
                bitmap.bearingX / ppem,
                -bitmap.bearingY / ppem,
                bitmap.width / ppem,
                bitmap.height / ppem,
            };

            g.push();
            g.fillStyle(*image);
            g.beginPath();
            g.rect(bound);
            g.fill();
            g.pop();
            return;
        }

        Gfx::Fontface::paintGlyph(g, glyph);
    }
};

} // namespace Karm::Font::Ttf
