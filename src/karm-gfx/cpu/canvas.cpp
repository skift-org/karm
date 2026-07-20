module;

#include <karm/macros>

export module Karm.Gfx:cpu.canvas;

import Karm.Core;
import Karm.Math;
import :buffer;
import :canvas;
import :fill;
import :filters;
import :stroke;
import :cpu.rast;

namespace Karm::Gfx {

using _CpuFills = Union<
    Color,
    Gradient,
    Pixels>;


export struct CpuFill : _CpuFills {
    using _CpuFills::_CpuFills;

    static CpuFill from(Fill fill) {
        return fill.visit(
            [](Color const& c) -> CpuFill {
                return c;
            },
            [](Gradient const& g) -> CpuFill {
                return g;
            },
            [](Rc<Image> const& img) -> CpuFill {
                return img->pixels();
            }
        );
    }

    always_inline Color sample(Math::Vec2f pos) const {
        return visit(
            [&](auto const& p) {
                return p.sample(pos);
            }
        );
    }
};

export struct CpuCanvas : Canvas {
    struct Scope {
        CpuFill fill = Gfx::WHITE;
        Stroke stroke{};
        Math::Recti clip{};
        Math::Trans2f trans = Math::Trans2f::identity();
        Opt<Rc<Image>> clipMask = NONE;
        Math::Recti clipBound = {0, 0};
        f64 opacity = 1.0;
    };

    static constexpr isize GLYPH_SUBPIXELS = 4;

    struct GlyphKey {
        Fontface const* face;
        Glyph glyph;
        f64 xScale;
        f64 yScale;
        u8 subpixel;

        void hash(Meta::Derive<Hasher> auto& h) const {
            Karm::hash(h, usize(face));
            Karm::hash(h, glyph);
            Karm::hash(h, xScale);
            Karm::hash(h, yScale);
            Karm::hash(h, subpixel);
        }

        bool operator==(GlyphKey const&) const = default;
    };

    struct CachedGlyph {
        Rc<Fontface> face;          //< Pins the face so the key's pointer stays valid
        Opt<Rc<Image>> mask = NONE; //< Coverage mask
        Math::Vec2i origin = {};    //< Top-left of the mask relative to the baseline
    };

    Opt<MutPixels> _pixels{};
    Vec<Scope> _stack{};
    Math::Path _path{};
    Math::Polyf _poly;
    CpuRast _rast{};
    Lru<GlyphKey, CachedGlyph> _glyphCache{4096};

    // MARK: Buffers -----------------------------------------------------------

    // Begin drawing operations on the given pixels.
    void begin(MutPixels p) {
        _pixels = p;
        _stack.pushBack({
            .clip = pixels().bound(),
        });
    }

    // End drawing operations.
    void end() {
        if (_stack.len() != 1) [[unlikely]]
            panic("save/restore mismatch");
        _stack.popBack();
        _pixels = NONE;
    }

    // Get the pixels being drawn on.
    MutPixels mutPixels() {
        return _pixels.unwrap("no pixels");
    }

    // Get the pixels being drawn on.
    Pixels pixels() const {
        return _pixels.unwrap("no pixels");
    }

    // Get the current scope.
    Scope& current() {
        return last(_stack);
    }

    // Get the current scope.
    Scope const& current() const {
        return last(_stack);
    }

    // MARK: Context Operations ------------------------------------------------

    void push() override {
        if (_stack.len() > 100) [[unlikely]]
            panic("context stack overflow");

        _stack.pushBack(current());
    }

    void pop() override {
        if (_stack.len() == 1) [[unlikely]]
            panic("context without save");

        _stack.popBack();
    }

    void fillStyle(Fill style) override {
        current().fill = CpuFill::from(style);
    }

    void strokeStyle(Stroke style) override {
        current().stroke = style;
    }

    void opacity(f64 opacity) override {
        current().opacity = opacity;
    }

    void transform(Math::Trans2f trans) override {
        auto& t = current().trans;
        t = trans.multiply(t);
    }

    // MARK: Path Operations ---------------------------------------------------

    // (internal) Fill the current shape with the given fill.
    // NOTE: The shape must be flattened before calling this function.
    void _fillImpl(auto fill, auto format, FillRule fillRule) {
        auto opacity = current().opacity;
        if (current().clipMask.has()) {
            auto& clipMask = *current().clipMask.unwrap();
            _rast.fill(_poly, current().clip, fillRule, [&](CpuRast::Frag frag) {
                u8 const mask = clipMask.pixels().loadUnsafe(frag.xy - current().clipBound.xy).red;

                auto* pixel = mutPixels().pixelUnsafe(frag.xy);
                auto color = fill.sample(frag.uv);
                color.alpha = Math::roundi(color.alpha * frag.a * (mask / 255.0) * opacity);
                auto c = format.load(pixel);
                c = color.blendOver(c);
                format.store(pixel, c);
            });
        } else
            _rast.fill(_poly, current().clip, fillRule, [&](CpuRast::Frag frag) {
                auto* pixel = mutPixels().pixelUnsafe(frag.xy);
                auto color = fill.sample(frag.uv);
                color.alpha = Math::roundi(color.alpha * frag.a * opacity);
                auto c = format.load(pixel);
                c = color.blendOver(c);
                format.store(pixel, c);
            });
    }

    void _fill(CpuFill fill, FillRule rule = FillRule::NONZERO) {
        fill.visit([&](auto fill) {
            pixels().fmt().visit([&](auto format) {
                _fillImpl(fill, format, rule);
            });
        });
    }

    void beginPath() override {
        _path.clear();
    }

    void closePath() override {
        _path.close();
    }

    void moveTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _path.moveTo(p, options);
    }

    void lineTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _path.lineTo(p, options);
    }

    void hlineTo(f64 x, Flags<Math::Path::Option> options) override {
        _path.hlineTo(x, options);
    }

    void vlineTo(f64 y, Flags<Math::Path::Option> options) override {
        _path.vlineTo(y, options);
    }

    void cubicTo(Math::Vec2f cp1, Math::Vec2f cp2, Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _path.cubicTo(cp1, cp2, p, options);
    }

    void quadTo(Math::Vec2f cp, Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _path.quadTo(cp, p, options);
    }

    void arcTo(Math::Vec2f radii, f64 angle, Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _path.arcTo(radii, angle, p, options);
    }

    void line(Math::Edgef line) override {
        _path.line(line);
    }

    void curve(Math::Curvef curve) override {
        _path.curve(curve);
    }

    void rect(Math::Rectf rect, Math::Radiif radii) override {
        _path.rect(rect, radii);
    }

    void arc(Math::Arcf arc) override {
        _path.arc(arc);
    }

    void path(Math::Path const& path) override {
        _path.path(path);
    }

    void ellipse(Math::Ellipsef ellipse) override {
        _path.ellipse(ellipse);
    }

    void fill(FillRule rule) override {
        _poly.clear();
        createSolid(_poly, _path);
        _poly.transform(current().trans);
        _poly.sortForAet();
        _fill(current().fill, rule);
    }

    void stroke() override {
        _poly.clear();
        createStroke(_poly, _path, current().stroke);
        _poly.transform(current().trans);
        _poly.sortForAet();
        _fill(CpuFill::from(current().stroke.fill));
    }

    void clip(FillRule rule) override {
        _poly.clear();
        createSolid(_poly, _path);
        _poly.transform(current().trans);
        _poly.sortForAet();

        auto clipBound = _poly.bound().ceil().cast<isize>().clipTo(current().clip);

        Rc<Image> newClipMask = Image::alloc(clipBound.wh, Gfx::GREYSCALE8);

        current().clip = clipBound;
        _rast.fill(_poly, current().clip, rule, [&](CpuRast::Frag frag) {
            u8 const parentPixel = current().clipMask.has() ? current().clipMask.unwrap()->pixels().load(frag.xy - current().clipBound.xy).red : 255;
            newClipMask->mutPixels().store(frag.xy - clipBound.xy, Color::fromRgb(Math::roundi(parentPixel * frag.a), 0, 0));
        });

        current().clipMask = newClipMask;
        current().clipBound = clipBound;
    }

    // MARK: Shape Operations --------------------------------------------------

    void _fillRect(Math::Recti r, Gfx::Color color) {
        // FIXME: Properly handle offaxis rectangles
        r = current().trans.apply(r.cast<f64>()).bound().cast<isize>();

        r = current().clip.clipTo(r);

        if (color.alpha == 255) {
            mutPixels()
                .clip(r)
                .clear(color);
        } else {
            pixels().fmt().visit([&](auto f) {
                for (isize y = r.y; y < r.y + r.height; ++y) {
                    for (isize x = r.x; x < r.x + r.width; ++x) {
                        auto blended = color.blendOver(f.load(mutPixels().pixelUnsafe({x, y})));
                        f.store(mutPixels().pixelUnsafe({x, y}), blended);
                    }
                }
            });
        }
    }

    void fill(Math::Recti r, Math::Radiif radii) override {
        beginPath();
        rect(r.cast<f64>(), radii);

        bool isSuitableForFastFill =
            radii.zero() and
            current().fill.is<Color>() and
            current().trans.axisAligned() and
            not current().clipMask.has() and
            current().opacity > 0.99;

        if (isSuitableForFastFill) {
            _fillRect(r, current().fill.unwrap<Color>());
        } else {
            fill(FillRule::NONZERO);
        }
    }

    void clip(Math::Recti r) override {
        if (current().trans.isIdentity()) {
            current().clip = r.clipTo(current().clip);
            return;
        }
        clip(r.cast<f64>());
    }

    void clip(Math::Rectf r) override {
        if (current().trans.axisAligned()) {
            r = current().trans.apply(r).bound();
            current().clip = r.cast<isize>().clipTo(current().clip);
            return;
        }

        beginPath();
        rect(r, {});
        clip(FillRule::EVENODD);
    }

    void stroke(Math::Path const& path) override {
        _poly.clear();
        createStroke(_poly, path, current().stroke);
        _poly.transform(current().trans);
        _poly.sortForAet();
        _fill(CpuFill::from(current().stroke.fill));
    }

    void fill(Math::Path const& path, FillRule rule = FillRule::NONZERO) override {
        _poly.clear();
        createSolid(_poly, path);
        _poly.transform(current().trans);
        _poly.sortForAet();
        _fill(current().fill, rule);
    }

    // MARK: Glyph Operations --------------------------------------------------

    CachedGlyph _renderGlyph(Font const& font, Glyph glyph, Math::Vec2f scale, u8 subpixel) {
        push();
        current().trans = Math::Trans2f::scale(scale);
        beginPath();
        font.fontface->contour(*this, glyph);

        _poly.clear();
        createSolid(_poly, _path);
        _poly.transform(current().trans);
        pop();

        _poly.offset({subpixel / (f64)GLYPH_SUBPIXELS, 0.0});
        _poly.sortForAet();

        if (not _poly.len())
            return {font.fontface};

        auto bound = _poly.bound().grow(1.0).ceil().cast<isize>();
        if (bound.width <= 0 or bound.height <= 0)
            return {font.fontface};

        auto mask = Image::alloc(bound.wh, GREYSCALE8);
        auto maskPixels = mask->mutPixels();
        maskPixels.clear();

        _rast.fill(_poly, bound, FillRule::NONZERO, [&](CpuRast::Frag frag) {
            maskPixels.store(frag.xy - bound.xy, Color::fromRgb(Math::roundi(frag.a * 255), 0, 0));
        });

        return {font.fontface, mask, bound.xy};
    }

    void _blitGlyph(CachedGlyph const& cached, Math::Vec2i baseline) {
        auto color = current().fill.unwrap<Color>();
        auto opacity = current().opacity;
        if (opacity < 0.001)
            return;

        auto src = cached.mask.unwrap()->pixels();
        auto destRect = Math::Recti{baseline + cached.origin, src.size()};
        auto clipped = current().clip.clipTo(destRect);
        if (clipped.width <= 0 or clipped.height <= 0)
            return;

        pixels().fmt().visit([&](auto format) {
            for (isize y = clipped.top(); y < clipped.bottom(); y++) {
                for (isize x = clipped.start(); x < clipped.end(); x++) {
                    Math::Vec2i pos = {x, y};
                    u8 cov = src.loadUnsafe(pos - destRect.xy).red;
                    if (cov == 0)
                        continue;

                    f64 factor = opacity;
                    if (current().clipMask.has())
                        factor *= current().clipMask.unwrap()->pixels().loadUnsafe(pos - current().clipBound.xy).red / 255.0;

                    auto* px = mutPixels().pixelUnsafe(pos);
                    auto c = format.load(px);

                    auto tint = color;
                    tint.alpha = Math::roundi(color.alpha * (cov / 255.0) * factor);
                    c = tint.blendOver(c);

                    format.store(px, c);
                }
            }
        });
    }

    void fill(Font const& font, Glyph glyph, Math::Vec2f baseline) override {
        auto& trans = current().trans;

        bool cacheable =
            current().fill.is<Color>() and
            trans.axisAligned() and
            trans.xx > 0 and
            trans.yy > 0;

        if (not cacheable) {
            Canvas::fill(font, glyph, baseline);
            return;
        }

        Math::Vec2f scale = {trans.xx * font.fontsize, trans.yy * font.fontsize};
        auto pos = trans.apply(baseline);
        Math::Vec2i ipos = {Math::floori(pos.x), Math::roundi(pos.y)};
        u8 subpixel = clamp<isize>(Math::floori((pos.x - ipos.x) * GLYPH_SUBPIXELS), 0, GLYPH_SUBPIXELS - 1);

        auto& cached = _glyphCache.access(
            GlyphKey{&font.fontface.unwrap(), glyph, scale.x, scale.y, subpixel},
            [&] {
                return _renderGlyph(font, glyph, scale, subpixel);
            }
        );

        if (cached.mask.has())
            _blitGlyph(cached, ipos);
    }

    // MARK: Clear Operations --------------------------------------------------

    void clear(Color color = BLACK) override {
        mutPixels()
            .clip(current().clip)
            .clear(color);
    }

    void clear(Math::Recti rect, Color color = BLACK) override {
        // FIXME: Properly handle offaxis rectangles
        rect = current().trans.apply(rect.cast<f64>()).bound().cast<isize>();

        rect = current().clip.clipTo(rect);
        mutPixels()
            .clip(rect)
            .clear(color);
    }

    // MARK: Plot Operations ---------------------------------------------------

    void plot(Math::Vec2i point, Color color) override {
        point = current().trans.apply(point.cast<f64>()).cast<isize>();
        if (current().clip.contains(point)) {
            mutPixels().blend(point, color);
        }
    }

    void plot(Math::Edgei edge, Color color) override {
        isize dx = Math::abs(edge.ex - edge.sx);
        isize sx = edge.sx < edge.ex ? 1 : -1;

        isize dy = -Math::abs(edge.ey - edge.sy);
        isize sy = edge.sy < edge.ey ? 1 : -1;

        isize err = dx + dy, e2;

        for (;;) {
            plot(edge.start, color);
            if (edge.sx == edge.ex and edge.sy == edge.ey)
                break;
            e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                edge.sx += sx;
            }
            if (e2 <= dx) {
                err += dx;
                edge.sy += sy;
            }
        }
    }

    void plot(Math::Recti rect, Color color) override {
        rect = {rect.xy, rect.wh - 1};
        plot(Math::Edgei{rect.topStart(), rect.topEnd()}, color);
        plot(Math::Edgei{rect.topEnd(), rect.bottomEnd()}, color);
        plot(Math::Edgei{rect.bottomEnd(), rect.bottomStart()}, color);
        plot(Math::Edgei{rect.bottomStart(), rect.topStart()}, color);
    }

    // MARK: Blit Operations ---------------------------------------------------

    void _blit(
        Pixels src,
        Math::Recti srcRect,
        auto srcFmt,

        MutPixels dest,
        Math::Recti destRect,
        auto destFmt
    ) {
        // FIXME: Properly handle offaxis rectangles
        destRect = current().trans.apply(destRect.cast<f64>()).bound().cast<isize>();

        auto clipDest = current().clip.clipTo(destRect);

        auto hratio = srcRect.height / (f64)destRect.height;
        auto wratio = srcRect.width / (f64)destRect.width;

        for (isize y = 0; y < clipDest.height; ++y) {
            isize yy = clipDest.y - destRect.y + y;

            auto srcY = srcRect.y + yy * hratio;
            auto destY = clipDest.y + y;

            for (isize x = 0; x < clipDest.width; ++x) {
                isize xx = clipDest.x - destRect.x + x;

                auto srcX = srcRect.x + xx * wratio;
                auto destX = clipDest.x + x;

                u8 const* srcPx = static_cast<u8 const*>(src.pixelUnsafe({(isize)srcX, (isize)srcY}));
                u8* destPx = static_cast<u8*>(dest.pixelUnsafe({destX, destY}));
                auto srcC = srcFmt.load(srcPx);
                auto destC = destFmt.load(destPx);
                destFmt.store(destPx, srcC.blendOver(destC));
            }
        }
    }

    void blit(Math::Recti src, Math::Recti dest, Rc<Image> surface) override {
        auto d = mutPixels();
        auto pixels = surface->pixels();
        d.fmt().visit([&](auto dfmt) {
            pixels.fmt().visit([&](auto pfmt) {
                _blit(pixels, src, pfmt, d, dest, dfmt);
            });
        });
    }

    // MARK: Filter Operations -------------------------------------------------

    void apply(Filter filter) override {
        apply(filter, pixels().bound());
    }

    void apply(Filter filter, Math::Recti region) {
        // FIXME: Properly handle offaxis rectangles
        region = current().trans.apply(region.cast<f64>()).bound().cast<isize>();
        region = current().clip.clipTo(region);
        filter.apply(mutPixels().clip(region));
    }
};

} // namespace Karm::Gfx
