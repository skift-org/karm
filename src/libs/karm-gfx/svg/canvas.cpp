module;

#include "../canvas.h"

#include "../prose.h"

export module Karm.Gfx:svg.canvas;

import Karm.Core;

namespace Karm::Gfx {

export struct SvgCanvas : Canvas {
    struct State {
        Fill fill{};
        Stroke stroke{};
        f64 opacity{1.0};
        StringBuilder transform; // accumulated transform attribute
        FillRule fillRule{FillRule::NONZERO};
    };

    StringBuilder _sb;
    StringBuilder _path; // current path data
    Vec<State> _stack;
    State _state{};

    // ------------------------------------------------------------
    // utils
    static String fmtF64(f64 v) {
        // Trim trailing zeros for tighter SVG
        return Io::format("{}", v);
    }

    void openSvgIfNeeded() {
        if (isEmpty(_sb.str())) {
            _sb.append("<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.2\" shape-rendering=\"geometricPrecision\">\n"s);
        }
    }

    void closeSvgIfOpen() {
        while (_stack)
            pop();
        if (any(_sb.str())) {
            _sb.append("</svg>\n"s);
        }
    }

    static Str fillRuleToSvg(FillRule r) {
        switch (r) {
        case FillRule::NONZERO:
            return "nonzero";
        case FillRule::EVENODD:
            return "evenodd";
        default:
            return "nonzero";
        }
    }

    static Str lineCapToSvg(StrokeCap c) {
        switch (c) {
        case StrokeCap::BUTT_CAP:
            return "butt";
        case StrokeCap::ROUND_CAP:
            return "round";
        case StrokeCap::SQUARE_CAP:
            return "square";
        default:
            return "butt";
        }
    }

    static Str lineJoinToSvg(StrokeJoin j) {
        switch (j) {
        case StrokeJoin::MITER_JOIN:
            return "miter";
        case StrokeJoin::ROUND_JOIN:
            return "round";
        case StrokeJoin::BEVEL_JOIN:
            return "bevel";
        default:
            return "miter";
        }
    }

    static String colorToSvg(Color c, f64 alphaScale) {
        auto a = static_cast<f64>(c.alpha) / 255.0 * alphaScale;
        return Io::format("rgba({}, {}, {}, {})", c.red, c.green, c.blue, fmtF64(a));
    }

    void emitPath(Str extraAttrs = {}) {
        if (isEmpty(_path.str()))
            return;
        openSvgIfNeeded();
        _sb.append("  <path d=\""s);
        _sb.append(_path.str());
        _sb.append("\""s);
        if (any(_state.transform.str())) {
            _sb.append(" transform=\""s);
            _sb.append(_state.transform.str());
            _sb.append("\""s);
        }
        // style
        if (_state.opacity != 1.0) {
            _sb.append(" opacity=\""s);
            _sb.append(fmtF64(_state.opacity));
            _sb.append("\""s);
        }
        if (any(extraAttrs)) {
            _sb.append(' ');
            _sb.append(extraAttrs);
        }
        _sb.append("/>\n"s);
        _path.clear();
    }

    void emitGroupOpen() {
        openSvgIfNeeded();
        _sb.append("  <g"s);
        if (any(_state.transform.str())) {
            _sb.append(" transform=\""s);
            _sb.append(_state.transform.str());
            _sb.append("\""s);
        }
        if (_state.opacity != 1.0) {
            _sb.append(" opacity=\""s);
            _sb.append(fmtF64(_state.opacity));
            _sb.append("\""s);
        }
        _sb.append(">\n"s);
    }

    void emitGroupClose() {
        _sb.append("  </g>\n"s);
    }

    String finalize() {
        emitPath(); // flush any pending path
        closeSvgIfOpen();
        return _sb.take();
    }

    // ------------------------------------------------------------
    // state stack
    void push() override {
        _stack.pushBack(_state);
        emitGroupOpen();
    }

    void pop() override {
        emitPath();
        if (_stack) {
            _state = _stack.popBack();
            emitGroupClose();
        }
    }

    void fillStyle(Fill style) override { _state.fill = style; }

    void strokeStyle(Stroke style) override { _state.stroke = style; }

    void opacity(f64 o) override { _state.opacity = o; }

    // transforms aggregate as SVG transform attribute
    void appendTransformCmd(Str cmd) {
        if (any(_state.transform.str()))
            _state.transform.append(' ');
        _state.transform.append(cmd);
    }

    void origin(Math::Vec2f p) override {
        // origin is treated as translation
        appendTransformCmd(Io::format("translate({} {})", fmtF64(p.x), fmtF64(p.y)));
    }

    void transform(Math::Trans2f t) override {
        // assume Trans2f can provide matrix values a b c d e f
        auto m = t; // {a,b,c,d,e,f}
        appendTransformCmd(Io::format("matrix({} {} {} {} {} {})", fmtF64(m.xx), fmtF64(m.xy), fmtF64(m.yx), fmtF64(m.yy), fmtF64(m.ox), fmtF64(m.oy)));
    }

    void translate(Math::Vec2f p) override {
        appendTransformCmd(Io::format("translate({} {})", fmtF64(p.x), fmtF64(p.y)));
    }

    void scale(Math::Vec2f s) override {
        appendTransformCmd(Io::format("scale({} {})", fmtF64(s.x), fmtF64(s.y)));
    }

    void rotate(f64 angle) override {
        appendTransformCmd(Io::format("rotate({})", fmtF64(angle * 180.0 / Math::PI)));
    }

    void skew(Math::Vec2f k) override {
        appendTransformCmd(Io::format("skewX({})", fmtF64(k.x)));
        appendTransformCmd(Io::format("skewY({})", fmtF64(k.y)));
    }

    // ------------------------------------------------------------
    // path construction
    void beginPath() override { _path.clear(); }

    void closePath() override { _path.append('Z'); }

    static bool isRel(Flags<Math::Path::Option> opt) {
        return opt.has(Math::Path::RELATIVE);
    }

    void moveTo(Math::Vec2f p, Flags<Math::Path::Option> opt) override {
        _path.append(isRel(opt) ? 'm' : 'M');
        _path.append(Io::format("{} {} ", fmtF64(p.x), fmtF64(p.y)));
    }

    void lineTo(Math::Vec2f p, Flags<Math::Path::Option> opt) override {
        _path.append(isRel(opt) ? 'l' : 'L');
        _path.append(Io::format("{} {} ", fmtF64(p.x), fmtF64(p.y)));
    }

    void hlineTo(f64 x, Flags<Math::Path::Option> opt) override {
        _path.append(isRel(opt) ? 'h' : 'H');
        _path.append(Io::format("{} ", fmtF64(x)));
    }

    void vlineTo(f64 y, Flags<Math::Path::Option> opt) override {
        _path.append(isRel(opt) ? 'v' : 'V');
        _path.append(Io::format("{} ", fmtF64(y)));
    }

    void cubicTo(Math::Vec2f c1, Math::Vec2f c2, Math::Vec2f p, Flags<Math::Path::Option> opt) override {
        _path.append(isRel(opt) ? 'c' : 'C');
        _path.append(Io::format("{} {}, {} {}, {} {} ", fmtF64(c1.x), fmtF64(c1.y), fmtF64(c2.x), fmtF64(c2.y), fmtF64(p.x), fmtF64(p.y)));
    }

    void quadTo(Math::Vec2f c, Math::Vec2f p, Flags<Math::Path::Option> opt) override {
        _path.append(isRel(opt) ? 'q' : 'Q');
        _path.append(Io::format("{} {}, {} {} ", fmtF64(c.x), fmtF64(c.y), fmtF64(p.x), fmtF64(p.y)));
    }

    void arcTo(Math::Vec2f r, f64 angle, Math::Vec2f p, Flags<Math::Path::Option> opt) override {
        // SVG arc flags approximated from options: assume large-arc if option LARGE, sweep if CCW
        bool large = opt.has(Math::Path::Option::LARGE);
        bool sweep = opt.has(Math::Path::Option::SWEEP);
        _path.append(isRel(opt) ? 'a' : 'A');
        _path.append(Io::format("{} {} {} {} {} {} {} ", fmtF64(r.x), fmtF64(r.y), fmtF64(angle * 180.0 / Math::PI), large ? 1 : 0, sweep ? 1 : 0, fmtF64(p.x), fmtF64(p.y)));
    }

    // convenience primitives
    void line(Math::Edgef e) override {
        beginPath();
        moveTo(e.start, {});
        lineTo(e.end, {});
    }

    void curve(Math::Curvef curve) override {
        beginPath();
        moveTo(curve.a, {});
        cubicTo(curve.b, curve.c, curve.d, {});
    }

    void ellipse(Math::Ellipsef e) override {
        beginPath();
        moveTo({e.center.x + e.radii.x, e.center.y}, {});
        arcTo(e.radii, 0, {e.center.x - e.radii.x, e.center.y}, {Math::Path::Option::LARGE, Math::Path::Option::SWEEP});
        arcTo(e.radii, 0, {e.center.x + e.radii.x, e.center.y}, {Math::Path::Option::LARGE});
        closePath();
    }

    void arc(Math::Arcf) override {
        // FIXME
    }

    void path(Math::Path const& p) override {
        for (auto& contour : p.iterContours()) {
            moveTo(contour[0], {});
            for (auto& vert : next(contour)) {
                lineTo(vert, {});
            }
            closePath();
        }
    }

    // ------------------------------------------------------------
    // painting

    void fill(FillRule rule) override {
        _state.fillRule = rule;
        emitPath(
            Io::format(
                " fill=\"{}\" fill-rule=\"{}\" stroke=\"none\"",
                colorToSvg(_state.fill.unwrapOr<Color>(FUCHSIA), _state.opacity),
                fillRuleToSvg(rule)
            )
        );
    }

    void fill(Fill style, FillRule rule) override {
        _state.fill = style;
        fill(rule);
    }

    void stroke() override {
        auto const& s = _state.stroke;
        emitPath(
            Io::format(
                " fill=\"none\" stroke=\"{}\" stroke-width=\"{}\" stroke-linecap=\"{}\" stroke-linejoin=\"{}\"{}",
                colorToSvg(s.fill.unwrapOr<Color>(FUCHSIA), _state.opacity),
                fmtF64(s.width),
                lineCapToSvg(s.cap),
                lineJoinToSvg(s.join)
            )
        );
    }

    void stroke(Stroke style) override {
        _state.stroke = style;
        stroke();
    }

    void clip(FillRule rule) override {
        // Basic clip via <clipPath>. Emit a path and reference it.
        // To keep it simple and stateless, inline clipPath per use.
        static usize id = 0;
        auto cid = Io::format("c{}", id++);
        openSvgIfNeeded();
        _sb.append("  <clipPath id=\""s);
        _sb.append(cid);
        _sb.append("\">\n"s);
        _sb.append("    <path d=\""s);
        _sb.append(_path.str());
        _sb.append("\" fill-rule=\""s);
        _sb.append(fillRuleToSvg(rule));
        _sb.append("\"/>\n  </clipPath>\n"s);
        _path.clear();
        // Wrap subsequent content in a group with the clip-path
        _sb.append("  <g clip-path=\"url(#"s);
        _sb.append(cid);
        _sb.append(")\">\n"s);
        // Caller should manage closing via pop() or extra push/pop around clip area.
    }

    void apply(Filter) override {
        // Not implemented in barebones SVG backend.
    }

    // ------------------------------------------------------------
    // high-level shape helpers (stroke/fill specific overloads)

    void fill(Font&, Glyph, Math::Vec2f) override {
        // Text shaping to SVG is out of scope for this minimal backend.
    }

    static inline void appendXmlEscapedRune(StringBuilder& out, u32 r) {
        if (r == '&') {
            out.append("&amp;"s);
        } else if (r == '<') {
            out.append("&lt;"s);
        } else if (r == '>') {
            out.append("&gt;"s);
        } else if (r == '"') {
            out.append("&quot;"s);
        } else if (r == '\'') {
            out.append("&apos;"s);
        } else {
            out.append(r);
        }
    }

    void fill(Prose& prose) override {
        emitPath();
        openSvgIfNeeded();
        _sb.append("  <g"s);
        if (any(_state.transform.str())) {
            _sb.append(" transform=\""s);
            _sb.append(_state.transform.str());
            _sb.append("\""s);
        }
        if (_state.opacity != 1.0) {
            _sb.append(" opacity=\""s);
            _sb.append(fmtF64(_state.opacity));
            _sb.append("\""s);
        }
        _sb.append(" fill=\""s);
        _sb.append(colorToSvg(prose._style.color.unwrapOr(FUCHSIA), _state.opacity));
        _sb.append("\""s);
        _sb.append(">"s);

        for (usize i = 0; i < prose._lines.len(); ++i) {
            auto const& line = prose._lines[i];
            if (not line.blocks())
                continue;

            for (auto& block : line.blocks()) {
                for (auto& cell : block.cells()) {
                    if (cell.strut())
                        continue;

                    _sb.append("    <text xml:space=\"preserve\" dominant-baseline=\"alphabetic\" x=\""s);
                    _sb.append(fmtF64((block.pos + cell.pos).cast<f64>()));
                    _sb.append("\" y=\""s);
                    _sb.append(fmtF64(line.baseline.cast<f64>()));
                    _sb.append("\">"s);
                    for (auto& r : cell.runes())
                        appendXmlEscapedRune(_sb, r);
                    _sb.append("</text>"s);
                }
            }
        }

        _sb.append("  </g>"s);
    }

    // ------------------------------------------------------------
    // pixel ops -> approximations or no-ops in SVG
    void clear(Color color) override {
        openSvgIfNeeded();
        _sb.append("  <rect x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" fill=\""s);
        _sb.append(colorToSvg(color, 1.0));
        _sb.append("\"/>\n"s);
    }

    void clear(Math::Recti r, Color color) override {
        openSvgIfNeeded();
        _sb.append(Io::format("  <rect x=\"{}\" y=\"{}\" width=\"{}\" height=\"{}\" fill=\"{}\"/>\n", r.x, r.y, r.width, r.height, colorToSvg(color, 1.0)));
    }

    void plot(Math::Vec2i p, Color c) override {
        openSvgIfNeeded();
        _sb.append(Io::format("  <rect x=\"{}\" y=\"{}\" width=\"1\" height=\"1\" fill=\"{}\"/>\n", p.x, p.y, colorToSvg(c, 1.0)));
    }

    void plot(Math::Edgei e, Color c) override {
        openSvgIfNeeded();
        _sb.append(Io::format("  <line x1=\"{}\" y1=\"{}\" x2=\"{}\" y2=\"{}\" stroke=\"{}\" stroke-width=\"1\"/>\n", e.start.x, e.start.y, e.end.x, e.end.y, colorToSvg(c, 1.0)));
    }

    void plot(Math::Recti r, Color c) override { clear(r, c); }

    void blit(Math::Recti, Math::Recti, Pixels) override {}

    void blit(Math::Recti, Pixels) override {}

    void blit(Math::Vec2i, Pixels) override {}
};

} // namespace Karm::Gfx
