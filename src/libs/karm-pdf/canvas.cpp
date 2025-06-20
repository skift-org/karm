#include <karm-logger/logger.h>
#include <karm-text/prose.h>

#include "canvas.h"

namespace Karm::Pdf {

static constexpr bool DEBUG_CANVAS = false;

// MARK: Context Operations ------------------------------------------------

void Canvas::push() {
    _e.ln("q");
}

void Canvas::pop() {
    _e.ln("Q");
}

void Canvas::fillStyle(Gfx::Fill fill) {
    auto color = fill.unwrap<Gfx::Color>();
    if (color.alpha == 0)
        return;

    if (color.alpha == 255) {
        _e.ln("{:.3} {:.3} {:.3} rg", color.red / 255.0, color.green / 255.0, color.blue / 255.0);
        return;
    }

    _e.ln("/GS{} gs", _graphicalStates.len());
    _e.ln("{:.3} {:.3} {:.3} rg", color.red / 255.0, color.green / 255.0, color.blue / 255.0);
    _graphicalStates.pushBack(GraphicalStateDict{color.alpha / 255.0});
}

void Canvas::strokeStyle(Gfx::Stroke) {
    logDebugIf(DEBUG_CANVAS, "pdf: strokeStyle() operation not implemented");
}

void Canvas::opacity(f64) {
    logDebugIf(DEBUG_CANVAS, "pdf: opacity() operation not implemented");
}

void Canvas::transform(Math::Trans2f trans) {
    _e.ln("{} {} {} {} {} {} cm", trans.xx, trans.xy, trans.yx, trans.yy, trans.ox, trans.oy);
}

// MARK: Path Operations ---------------------------------------------------

void Canvas::beginPath() {
    _e.ln("n");
}

void Canvas::closePath() {
    _e.ln("h");
}

void Canvas::moveTo(Math::Vec2f p, Flags<Math::Path::Option> options) {
    p = _mapPointAndUpdate(p, options);
    _e.ln("{:.2} {:.2} m", p.x, p.y);
}

void Canvas::lineTo(Math::Vec2f p, Flags<Math::Path::Option> options) {
    p = _mapPointAndUpdate(p, options);
    _e.ln("{:.2} {:.2} l", p.x, p.y);
}

void Canvas::hlineTo(f64 x, Flags<Math::Path::Option> options) {
    auto p = _mapPoint({x, 0}, options);
    _e.ln("{:.2} 0 l", p.x);
}

void Canvas::vlineTo(f64 y, Flags<Math::Path::Option> options) {
    auto p = _mapPoint({0, y}, options);
    _e.ln("0 {:.2} l", p.y);
}

void Canvas::cubicTo(Math::Vec2f cp1, Math::Vec2f cp2, Math::Vec2f p, Flags<Math::Path::Option> options) {
    cp1 = _mapPoint(cp1, options);
    cp2 = _mapPoint(cp2, options);
    p = _mapPointAndUpdate(p, options);
    _e.ln("{:.2} {:.2} {:.2} {:.2} {:.2} {:.2} c", cp1.x, cp1.y, cp2.x, cp2.y, p.x, p.y);
}

void Canvas::quadTo(Math::Vec2f cp, Math::Vec2f p, Flags<Math::Path::Option> options) {
    auto curve = Math::Curvef::quadratic(_p, cp, p);
    cubicTo(curve.b, curve.c, curve.d, options);
}

void Canvas::arcTo(Math::Vec2f, f64, Math::Vec2f, Flags<Math::Path::Option>) {
    logDebugIf(DEBUG_CANVAS, "pdf: arcTo() operation not implemented");
}

void Canvas::line(Math::Edgef line) {
    moveTo(line.start, {});
    lineTo(line.end, {});
}

void Canvas::curve(Math::Curvef curve) {
    moveTo(curve.a, {});
    cubicTo(curve.b, curve.c, curve.d, {});
}

void Canvas::rect(Math::Rectf rect, Math::Radiif radii) {
    if (Math::epsilonEq(min(rect.width, rect.height), 0.0, 0.001))
        return;

    if (radii.zero()) {
        moveTo(rect.topStart(), {});
        lineTo(rect.topEnd(), {});
        lineTo(rect.bottomEnd(), {});
        lineTo(rect.bottomStart(), {});
        closePath();
    } else {
        radii = radii.reduceOverlap(rect.size());

        // NOTE: 0.5522847498 is the cubic bezier approximation of the circle
        //       Since we take the value relative to the end of the edge,
        //       we need to subtract it from the radii to get the control point
        f64 cpa = radii.a * (1 - 0.5522847498);
        f64 cpb = radii.b * (1 - 0.5522847498);
        f64 cpc = radii.c * (1 - 0.5522847498);
        f64 cpd = radii.d * (1 - 0.5522847498);
        f64 cpe = radii.e * (1 - 0.5522847498);
        f64 cpf = radii.f * (1 - 0.5522847498);
        f64 cpg = radii.g * (1 - 0.5522847498);
        f64 cph = radii.h * (1 - 0.5522847498);

        moveTo({rect.x + radii.b, rect.y}, {});

        // Top end edge
        lineTo({rect.x + rect.width - radii.c, rect.y}, {});
        cubicTo(
            {rect.x + rect.width - cpc, rect.y},
            {rect.x + rect.width, rect.y + cpd},
            {rect.x + rect.width, rect.y + radii.d}, {}
        );

        // Bottom end edge
        lineTo({rect.x + rect.width, rect.y + rect.height - radii.e}, {});
        cubicTo(
            {rect.x + rect.width, rect.y + rect.height - cpe},
            {rect.x + rect.width - cpf, rect.y + rect.height},
            {rect.x + rect.width - radii.f, rect.y + rect.height}, {}
        );

        // Bottom start edge
        lineTo({rect.x + radii.g, rect.y + rect.height}, {});
        cubicTo(
            {rect.x + cpg, rect.y + rect.height},
            {rect.x, rect.y + rect.height - cph},
            {rect.x, rect.y + rect.height - radii.h}, {}
        );

        // Top start edge
        lineTo({rect.x, rect.y + radii.a}, {});
        cubicTo(
            {rect.x, rect.y + cpa},
            {rect.x + cpb, rect.y},
            {rect.x + radii.b, rect.y}, {}
        );

        closePath();
    }
}

void Canvas::ellipse(Math::Ellipsef) {
    logDebugIf(DEBUG_CANVAS, "pdf: ellipse() operation not implemented");
}

void Canvas::arc(Math::Arcf) {
    logDebugIf(DEBUG_CANVAS, "pdf: arc() operation not implemented");
}

void Canvas::path(Math::Path const& path) {
    // FIXME: use list of ops
    for (auto& contour : path.iterContours()) {
        moveTo(contour[0], {});
        for (auto& vert : next(contour)) {
            lineTo(vert, {});
        }
        closePath();
    }
}

void Canvas::fill(Gfx::FillRule rule) {
    if (rule == Gfx::FillRule::NONZERO)
        _e.ln("f");
    else
        _e.ln("f*");
}

void Canvas::fill(Text::Prose& prose) {
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

void Canvas::fill(Gfx::Fill f, Gfx::FillRule rule) {
    push();
    fillStyle(f);
    fill(rule);
    pop();
}

void Canvas::stroke() {
    logDebugIf(DEBUG_CANVAS, "pdf: stroke() operation not implemented");
}

void Canvas::stroke(Gfx::Stroke style) {
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

void Canvas::clip(Gfx::FillRule rule) {
    if (rule == Gfx::FillRule::EVENODD)
        _e.ln("W* n");
    else
        _e.ln("W n");
}

void Canvas::apply(Gfx::Filter) {
    logDebugIf(DEBUG_CANVAS, "pdf: apply() operation not implemented");
};

// MARK: Clear Operations --------------------------------------------------

void Canvas::clear(Gfx::Color) {
    logDebugIf(DEBUG_CANVAS, "pdf: clear() operation not implemented");
}

void Canvas::clear(Math::Recti, Gfx::Color) {
    logDebugIf(DEBUG_CANVAS, "pdf: clear() operation not implemented");
}

// MARK: Plot Operations ---------------------------------------------------

void Canvas::plot(Math::Vec2i, Gfx::Color) {
    logDebugIf(DEBUG_CANVAS, "pdf: plot() operation not implemented");
}

void Canvas::plot(Math::Edgei, Gfx::Color) {
    logDebugIf(DEBUG_CANVAS, "pdf: plot() operation not implemented");
}

void Canvas::plot(Math::Recti, Gfx::Color) {
    logDebugIf(DEBUG_CANVAS, "pdf: plot() operation not implemented");
}

// MARK: Blit Operations ---------------------------------------------------

void Canvas::blit(Math::Recti, Math::Recti, Gfx::Pixels) {
    logDebugIf(DEBUG_CANVAS, "pdf: blit() operation not implemented");
}

} // namespace Karm::Pdf
