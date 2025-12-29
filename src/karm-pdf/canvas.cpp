export module Karm.Pdf:canvas;

import Karm.Core;
import Karm.Debug;
import Karm.Gfx;
import Karm.Math;
import Karm.Logger;

namespace Karm::Pdf {

static auto debugCanvas = Debug::Flag::debug("pdf-canvas", "Log PDF canvas failures");

export struct FontManager {
    Map<Gfx::FontAttrs, Tuple<usize, Rc<Gfx::Fontface>>> mapping;

    usize getFontId(Rc<Gfx::Fontface> font) {
        if (auto id = mapping.tryGet(font->attrs()))
            return id.unwrap().v0;

        auto id = mapping.len() + 1;
        mapping.put(font->attrs(), {id, font});
        return id;
    }
};

export struct ImageManager {
    Map<usize, Rc<Gfx::Surface>> mapping;

    usize getImageId(Rc<Gfx::Surface> surface) {
        if (auto storedSurface = mapping.tryGet(surface->id()))
            return storedSurface->id();

        auto id = surface.id();
        mapping.put(id, surface);
        return id;
    }
};

// 8.4.5 Graphics state parameter dictionaries
export struct GraphicalStateDict {
    f64 opacity;
};

export struct Canvas : Gfx::Canvas {
    Io::Emit _e;
    Math::Vec2f _mediaBox{};
    Math::Vec2f _p{};

    MutCursor<FontManager> _fontManager;
    MutCursor<ImageManager> _imageManager;
    Vec<GraphicalStateDict>& _graphicalStates;

    Canvas(Io::Emit e, Math::Vec2f mediaBox, MutCursor<FontManager> fontManager, MutCursor<ImageManager> imageManager, Vec<GraphicalStateDict>& graphicalStates)
        : _e{e}, _mediaBox{mediaBox}, _fontManager{fontManager}, _imageManager{imageManager}, _graphicalStates(graphicalStates) {}

    Math::Vec2f _mapPoint(Math::Vec2f p, Flags<Math::Path::Option> options) {
        if (options & Math::Path::RELATIVE)
            return p + _p;
        return p;
    }

    Math::Vec2f _mapPointAndUpdate(Math::Vec2f p, Flags<Math::Path::Option> options) {
        if (options & Math::Path::RELATIVE)
            p = p + _p;
        _p = p;
        return p;
    }

    // MARK: Context Operations ------------------------------------------------

    void push() override {
        _e.ln("q");
    }

    void pop() override {
        _e.ln("Q");
    }

    void fillStyle(Gfx::Fill fill) override {
        auto color = fill.unwrap<Gfx::Color>();

        if (color.alpha == 255) {
            _e.ln("{:.3} {:.3} {:.3} rg", color.red / 255.0, color.green / 255.0, color.blue / 255.0);
            return;
        }

        _e.ln("/GS{} gs", _graphicalStates.len());
        _e.ln("{:.3} {:.3} {:.3} rg", color.red / 255.0, color.green / 255.0, color.blue / 255.0);
        _graphicalStates.pushBack(GraphicalStateDict{color.alpha / 255.0});
    }

    void strokeStyle(Gfx::Stroke) override {
        logDebugIf(debugCanvas, "pdf: strokeStyle() operation not implemented");
    }

    void opacity(f64) override {
        logDebugIf(debugCanvas, "pdf: opacity() operation not implemented");
    }

    void transform(Math::Trans2f trans) override {
        _e.ln("{} {} {} {} {} {} cm", trans.xx, trans.xy, trans.yx, trans.yy, trans.ox, trans.oy);
    }

    // MARK: Path Operations ---------------------------------------------------

    void beginPath() override {
        _e.ln("n");
    }

    void closePath() override {
        _e.ln("h");
    }

    void moveTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
        p = _mapPointAndUpdate(p, options);
        _e.ln("{:.2} {:.2} m", p.x, p.y);
    }

    void lineTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
        p = _mapPointAndUpdate(p, options);
        _e.ln("{:.2} {:.2} l", p.x, p.y);
    }

    void hlineTo(f64 x, Flags<Math::Path::Option> options) override {
        auto p = _mapPoint({x, 0}, options);
        _e.ln("{:.2} 0 l", p.x);
    }

    void vlineTo(f64 y, Flags<Math::Path::Option> options) override {
        auto p = _mapPoint({0, y}, options);
        _e.ln("0 {:.2} l", p.y);
    }

    void cubicTo(Math::Vec2f cp1, Math::Vec2f cp2, Math::Vec2f p, Flags<Math::Path::Option> options) override {
        cp1 = _mapPoint(cp1, options);
        cp2 = _mapPoint(cp2, options);
        p = _mapPointAndUpdate(p, options);
        _e.ln("{:.2} {:.2} {:.2} {:.2} {:.2} {:.2} c", cp1.x, cp1.y, cp2.x, cp2.y, p.x, p.y);
    }

    void quadTo(Math::Vec2f cp, Math::Vec2f p, Flags<Math::Path::Option> options) override {
        auto curve = Math::Curvef::quadratic(_p, cp, p);
        cubicTo(curve.b, curve.c, curve.d, options);
    }

    void arcTo(Math::Vec2f, f64, Math::Vec2f, Flags<Math::Path::Option>) override {
        logDebugIf(debugCanvas, "pdf: arcTo() operation not implemented");
    }

    void line(Math::Edgef line) override {
        moveTo(line.start, {});
        lineTo(line.end, {});
    }

    void curve(Math::Curvef curve) override {
        moveTo(curve.a, {});
        cubicTo(curve.b, curve.c, curve.d, {});
    }

    void ellipse(Math::Ellipsef) override {
        logDebugIf(debugCanvas, "pdf: ellipse() operation not implemented");
    }

    void arc(Math::Arcf) override {
        logDebugIf(debugCanvas, "pdf: arc() operation not implemented");
    }

    void path(Math::Path const& path) override {
        // FIXME: use list of ops
        for (auto& contour : path.iterContours()) {
            moveTo(contour[0], {});
            for (auto& vert : next(contour)) {
                lineTo(vert, {});
            }
            closePath();
        }
    }

    void fill(Gfx::FillRule rule) override {
        if (rule == Gfx::FillRule::NONZERO)
            _e.ln("f");
        else
            _e.ln("f*");
    }

    void fill(Gfx::Prose& prose) override {
        push();
        _e.ln("BT");
        _e.ln(
            "/F{} {} Tf",
            _fontManager->getFontId(prose._style.font.fontface),
            prose._style.font.fontSize()
        );

        if (prose._style.color)
            fillStyle(*prose._style.color);

        _e.ln("1 0 0 -1 0 {} Tm", prose._lineHeight * prose._lines.len());

        // TODO: this is needed since we are inverting the vertical axis in the PDF coordinate space
        reverse(mutSub(prose._lines));

        for (usize i = 0; i < prose._lines.len(); ++i) {
            auto const& line = prose._lines[i];

            if (not line.blocks())
                continue;

            auto alignedStart = first(line.blocks()).pos.cast<f64>();
            _e.ln("{} {} Td"s, alignedStart, i == 0 ? 0 : prose._lineHeight);

            auto prevEndPos = alignedStart;

            _e("[<");
            for (auto& block : line.blocks()) {
                for (auto& cell : block.cells()) {
                    if (cell.strut())
                        continue;
                    auto glyphAdvance = prose._style.font.advance(cell.glyph);
                    auto nextEndPosWithoutKern = prevEndPos + glyphAdvance;
                    auto nextDesiredEndPos = (block.pos + cell.pos + cell.adv).cast<f64>();

                    auto kern = nextEndPosWithoutKern - nextDesiredEndPos;
                    if (not Math::epsilonEq<f64>(kern, 0, 0.01))
                        _e(">{}<", kern);

                    for (auto rune : cell.runes()) {
                        _e("{04x}", rune);
                    }
                    prevEndPos = prevEndPos + glyphAdvance - kern;
                }
            }
            _e(">] TJ"s);
            _e.ln("");
        }

        _e.ln("ET");
        pop();
    }

    void fill(Gfx::Fill f, Gfx::FillRule rule) override {
        push();
        fillStyle(f);
        fill(rule);
        pop();
    }

    void stroke() override {
        logDebugIf(debugCanvas, "pdf: stroke() operation not implemented");
    }

    void stroke(Gfx::Stroke style) override {
        auto color = style.fill.unwrap<Gfx::Color>();
        _e.ln("{:.3} {:.3} {:.3} RG", color.red / 255., color.green / 255., color.blue / 255.);

        _e.ln("{:.2} w", style.width);

        if (style.cap == Gfx::ROUND_CAP)
            _e.ln("1 J");
        else if (style.cap == Gfx::SQUARE_CAP)
            _e.ln("2 J");
        else
            _e.ln("0 J");

        if (style.join == Gfx::ROUND_JOIN)
            _e.ln("1 j");
        else if (style.join == Gfx::BEVEL_JOIN)
            _e.ln("2 j");
        else
            _e.ln("0 j");

        _e.ln("S");
    }

    void clip(Gfx::FillRule rule) override {
        if (rule == Gfx::FillRule::EVENODD)
            _e.ln("W* n");
        else
            _e.ln("W n");
    }

    void apply(Gfx::Filter) override {
        logDebugIf(debugCanvas, "pdf: apply() operation not implemented");
    };

    // MARK: Clear Operations --------------------------------------------------

    void clear(Gfx::Color) override {
        logDebugIf(debugCanvas, "pdf: clear() operation not implemented");
    }

    void clear(Math::Recti, Gfx::Color) override {
        logDebugIf(debugCanvas, "pdf: clear() operation not implemented");
    }

    // MARK: Plot Operations ---------------------------------------------------

    void plot(Math::Vec2i, Gfx::Color) override {
        logDebugIf(debugCanvas, "pdf: plot() operation not implemented");
    }

    void plot(Math::Edgei, Gfx::Color) override {
        logDebugIf(debugCanvas, "pdf: plot() operation not implemented");
    }

    void plot(Math::Recti, Gfx::Color) override {
        logDebugIf(debugCanvas, "pdf: plot() operation not implemented");
    }

    // MARK: Blit Operations ---------------------------------------------------

    void blit(Math::Recti src, Math::Recti dest, Rc<Gfx::Surface> surface) override {
        (void)src; // FIXME

        push();
        _e.ln("{} 0 0 {} {} {} cm", dest.width, -dest.height, dest.x, dest.y + dest.height);

        _e.ln("/Im{} Do", _imageManager->getImageId(surface));

        pop();
    }

    // MARK: Filter Operations -------------------------------------------------
};

} // namespace Karm::Pdf
