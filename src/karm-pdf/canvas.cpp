export module Karm.Pdf:canvas;

import Karm.Core;
import Karm.Debug;
import Karm.Gfx;
import Karm.Math;
import Karm.Logger;

using namespace Karm::Literals;

namespace Karm::Pdf {

static auto debugCanvas = Debug::Flag::debug("pdf-canvas", "Log PDF canvas failures");

export struct FontManager {
    Map<Gfx::FontAttrs, Tuple<usize, Rc<Gfx::Fontface>>> mapping;

    usize getFontId(Rc<Gfx::Fontface> font) {
        if (auto id = mapping.lookup(font->attrs()))
            return id.unwrap().v0;

        auto id = mapping.len() + 1;
        mapping.put(font->attrs(), Tuple{id, font});
        return id;
    }
};

export struct ImageManager {
    Map<usize, Tuple<usize, Rc<Gfx::Image>>> mapping;

    usize getImageId(Rc<Gfx::Image> image) {
        if (auto entry = mapping.lookup(reinterpret_cast<usize>(image._cell)))
            return entry.unwrap().v0;

        auto id = mapping.len() + 1;
        mapping.put(reinterpret_cast<usize>(image._cell), Tuple{id, image});
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

    Vec<f64> _opacityStack;

    Canvas(Io::Emit e, Math::Vec2f mediaBox, MutCursor<FontManager> fontManager, MutCursor<ImageManager> imageManager, Vec<GraphicalStateDict>& graphicalStates)
        : _e{e}, _mediaBox{mediaBox}, _fontManager{fontManager}, _imageManager{imageManager}, _graphicalStates(graphicalStates) {
        _opacityStack.pushBack(1.0);
    }

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
        f64 opacity = last(_opacityStack);
        _opacityStack.pushBack(opacity);
    }

    void pop() override {
        _e.ln("Q");
        if (_opacityStack.len() > 1)
            _opacityStack.popBack();
    }

    void _applyOpacity(f64 opacity) {
        usize gsIndex = _graphicalStates.len();
        for (usize i = 0; i < _graphicalStates.len(); ++i) {
            if (Math::epsilonEq(_graphicalStates[i].opacity, opacity, 0.001)) {
                gsIndex = i;
                break;
            }
        }

        if (gsIndex == _graphicalStates.len()) {
            _graphicalStates.pushBack(GraphicalStateDict{opacity});
        }

        _e.ln("/GS{} gs", gsIndex);
    }

    void fillStyle(Gfx::Fill fill) override {
        auto color = fill.unwrap<Gfx::Color>();

        _e.ln("{:.3} {:.3} {:.3} rg", color.red / 255.0, color.green / 255.0, color.blue / 255.0);

        _applyOpacity(color.alpha / 255.0 * last(_opacityStack));
    }

    void strokeStyle(Gfx::Stroke) override {
        logDebugIf(debugCanvas, "pdf: strokeStyle() operation not implemented");
    }

    void opacity(f64 opacity) override {
        last(_opacityStack) = opacity;
        _applyOpacity(opacity);
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
            if (contour.close)
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

        Opt<Gfx::Color> currentColor = NONE;
        Opt<Gfx::Font> currentFont = NONE;

        for (usize i = 0; i < prose._lines.len(); ++i) {
            auto const& line = prose._lines[i];

            if (not line.blocks())
                continue;

            auto lineStartPos = first(line.blocks()).pos.cast<f64>();
            auto lineBaseline = line.baseline.cast<f64>();

            _e.ln("1 0 0 -1 {} {} Tm", lineStartPos, lineBaseline);

            auto prevEndPos = lineStartPos;
            bool inArray = false;

            auto ensureState = [&](Gfx::SpanStyle const& style) {
                bool changed =
                    not currentColor or *currentColor != style.color or
                    not currentFont or currentFont->fontface._cell != style.font.fontface._cell or
                    currentFont->fontsize != style.font.fontsize;

                if (changed) {
                    if (inArray) {
                        _e.ln(">] TJ"s);
                        inArray = false;
                    }
                    currentColor = style.color;
                    currentFont = style.font;
                    fillStyle(*currentColor);
                    _e.ln(
                        "/F{} {} Tf",
                        _fontManager->getFontId(currentFont->fontface),
                        currentFont->fontsize
                    );
                }

                if (not inArray) {
                    _e("[<");
                    inArray = true;
                }
            };

            for (auto& block : line.blocks()) {
                for (auto& cell : block.cells()) {
                    if (cell.type() == Gfx::Prose::CellType::STRUT)
                        continue;

                    if (cell.type() == Gfx::Prose::CellType::SPACER) {
                        ensureState(cell.style());
                        f64 pdfAdv = cell.adv.cast<f64>() * (1000.0 / currentFont->fontsize);
                        _e(">{}<", -pdfAdv);
                        prevEndPos = prevEndPos + cell.adv.cast<f64>();
                        continue;
                    }

                    ensureState(cell.style());

                    f64 fontSize = currentFont->fontsize;
                    auto glyphAdvance = currentFont->advance(cell.glyph);
                    auto nextEndPosWithoutKern = prevEndPos + glyphAdvance;
                    auto nextDesiredEndPos = (block.pos + cell.pos + cell.adv).cast<f64>();

                    auto kernDiff = nextEndPosWithoutKern - nextDesiredEndPos;

                    if (not Math::epsilonEq<f64>(kernDiff, 0, 0.01)) {
                        f64 pdfKern = kernDiff * (1000.0 / fontSize);
                        _e(">{}<", pdfKern);
                    }

                    for (auto rune : cell.runes()) {
                        _e("{:04x}", rune == '\n' ? ' ' : rune);
                    }

                    prevEndPos = prevEndPos + glyphAdvance - kernDiff;
                }
            }

            if (inArray)
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

    void blit(Math::Recti src, Math::Recti dest, Rc<Gfx::Image> image) override {
        if (src != dest)
            logWarn("image clipping is not implemented");

        auto destf = dest.cast<f64>();

        push();

        transform({destf.width, 0, 0, -destf.height, destf.x, destf.y + dest.height});

        _e.ln("/Im{} Do", _imageManager->getImageId(image));

        pop();
    }

    // MARK: Filter Operations -------------------------------------------------
};

} // namespace Karm::Pdf
