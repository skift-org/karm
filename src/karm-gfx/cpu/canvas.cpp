export module Karm.Gfx:cpu.canvas;

import Karm.Math;
import :buffer;
import :canvas;
import :fill;
import :filters;
import :stroke;
import :cpu.rast;

namespace Karm::Gfx {

export struct LcdLayout {
    Math::Vec2f red;
    Math::Vec2f green;
    Math::Vec2f blue;
};

export LcdLayout RGB = {{+0.33, 0.0}, {0.0, 0.0}, {-0.33, 0.0}};
export LcdLayout BGR = {{-0.33, 0.0}, {0.0, 0.0}, {+0.33, 0.0}};
export LcdLayout VRGB = {{0.0, +0.33}, {0.0, 0.0}, {0.0, -0.33}};

export struct CpuCanvas : Canvas {
    struct Scope {
        Fill fill = Gfx::WHITE;
        Stroke stroke{};
        Math::Recti clip{};
        Math::Trans2f trans = Math::Trans2f::identity();
        Opt<Rc<Surface>> clipMask = NONE;
        Math::Recti clipBound = {0, 0};
        float opacity = 1.0;
    };

    Opt<MutPixels> _pixels{};
    Vec<Scope> _stack{};
    Math::Path _path{};
    Math::Polyf _poly;
    CpuRast _rast{};
    LcdLayout _lcdLayout = RGB;
    bool _useSpaa = false;

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
        current().fill = style;
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
                color.alpha *= frag.a * (mask / 255.0) * opacity;
                auto c = format.load(pixel);
                c = color.blendOver(c);
                format.store(pixel, c);
            });
        } else
            _rast.fill(_poly, current().clip, fillRule, [&](CpuRast::Frag frag) {
                auto* pixel = mutPixels().pixelUnsafe(frag.xy);
                auto color = fill.sample(frag.uv);
                color.alpha *= frag.a * opacity;
                auto c = format.load(pixel);
                c = color.blendOver(c);
                format.store(pixel, c);
            });
    }

    void _FillSmoothImpl(auto fill, auto format, FillRule fillRule) {
        Math::Vec2f last = {0, 0};
        auto opacity = current().opacity;
        if (opacity < 0.001)
            return;
        auto fillComponent = [&](auto comp, Math::Vec2f pos) {
            _poly.offset(pos - last);
            last = pos;

            if (current().clipMask.has()) {
                auto& clipMask = *current().clipMask.unwrap();
                _rast.fill(_poly, current().clip, fillRule, [&](CpuRast::Frag frag) {
                    u8 mask = clipMask.pixels().loadUnsafe(frag.xy - current().clipBound.xy).red;

                    auto pixel = mutPixels().pixelUnsafe(frag.xy);
                    auto color = fill.sample(frag.uv);
                    color.alpha *= frag.a * (mask / 255.0) * opacity;
                    auto c = format.load(pixel);
                    c = color.blendOverComponent(c, comp);
                    format.store(pixel, c);
                });
            } else
                _rast.fill(_poly, current().clip, fillRule, [&](CpuRast::Frag frag) {
                    auto pixel = mutPixels().pixelUnsafe(frag.xy);
                    auto color = fill.sample(frag.uv);
                    color.alpha *= frag.a * opacity;
                    auto c = format.load(pixel);
                    c = color.blendOverComponent(c, comp);
                    format.store(pixel, c);
                });
        };

        fillComponent(Color::RED_COMPONENT, _lcdLayout.red);
        fillComponent(Color::GREEN_COMPONENT, _lcdLayout.green);
        fillComponent(Color::BLUE_COMPONENT, _lcdLayout.blue);
    }

    void _fill(Fill fill, FillRule rule = FillRule::NONZERO) {
        fill.visit([&](auto fill) {
            pixels().fmt().visit([&](auto format) {
                if (_useSpaa)
                    _FillSmoothImpl(fill, format, rule);
                else
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
        _fill(current().fill, rule);
    }

    void stroke() override {
        _poly.clear();
        createStroke(_poly, _path, current().stroke);
        _poly.transform(current().trans);
        _fill(current().stroke.fill);
    }

    void clip(FillRule rule) override {
        _poly.clear();
        createSolid(_poly, _path);
        _poly.transform(current().trans);

        auto clipBound = _poly.bound().ceil().cast<isize>().clipTo(current().clip);

        Rc<Surface> newClipMask = Surface::alloc(clipBound.wh, Gfx::GREYSCALE8);

        current().clip = clipBound;
        _rast.fill(_poly, current().clip, rule, [&](CpuRast::Frag frag) {
            u8 const parentPixel = current().clipMask.has() ? current().clipMask.unwrap()->pixels().load(frag.xy - current().clipBound.xy).red : 255;
            newClipMask->mutPixels().store(frag.xy - clipBound.xy, Color::fromRgb(parentPixel * frag.a, 0, 0));
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
            current().trans.isIdentity();

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
        if (current().trans.simple()) {
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
        _fill(current().stroke.fill);
    }

    void fill(Math::Path const& path, FillRule rule = FillRule::NONZERO) override {
        _poly.clear();
        createSolid(_poly, path);
        _poly.transform(current().trans);
        _fill(current().fill, rule);
    }

    void fill(Font& font, Glyph glyph, Math::Vec2f baseline) override {
        _useSpaa = true;
        Canvas::fill(font, glyph, baseline);
        _useSpaa = false;
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

    void blit(Math::Recti src, Math::Recti dest, Pixels pixels) override {
        auto d = mutPixels();
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
