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
    struct Entry {
        usize id;
        Rc<Gfx::Fontface> fontface;
        Map<u16, u16> codes = {};
        Map<String, u16> missingCodes = {};
        Vec<Gfx::Glyph> glyphs = {};
        Vec<Vec<Rune>> texts = {};
    };

    Map<Gfx::FontAttrs, Entry> mapping;
    void const* _lastFace = nullptr;
    Entry* _last = nullptr;

    Entry& entry(Rc<Gfx::Fontface> font) {
        if (_last and _lastFace == font._cell)
            return *_last;

        auto attrs = font->attrs();
        if (not mapping.contains(attrs)) {
            Entry entry{.id = mapping.len() + 1, .fontface = font};
            entry.codes.put(0, 0);
            entry.glyphs.pushBack(Gfx::Glyph{.index = 0, .font = 0});
            entry.texts.pushBack({});
            mapping.put(attrs, std::move(entry));
        }

        auto found = mapping.lookup(attrs);
        _last = &*found;
        _lastFace = font._cell;
        return *_last;
    }

    usize getFontId(Rc<Gfx::Fontface> font) {
        return entry(font).id;
    }

    u16 code(Rc<Gfx::Fontface> font, Gfx::Glyph glyph, Slice<Rune> text) {
        auto& e = entry(font);

        Opt<String> missingKey = NONE;
        if (glyph.index == 0) {
            if (isEmpty(text))
                return 0;
            StringBuilder key;
            for (auto rune : text)
                key.append(rune);
            missingKey = Some(key.take());
            if (auto existing = e.missingCodes.lookup(*missingKey))
                return *existing;
        } else if (auto existing = e.codes.lookup(glyph.index)) {
            return *existing;
        }

        if (e.glyphs.len() > 0xffff)
            return 0;

        u16 code = e.glyphs.len();
        if (missingKey)
            e.missingCodes.put(*missingKey, code);
        else
            e.codes.put(glyph.index, code);
        e.glyphs.pushBack(glyph);
        e.texts.pushBack(Vec<Rune>{text});
        return code;
    }
};

export struct ImageManager {
    Map<usize, Tuple<usize, Rc<Gfx::Image>>> mapping;

    usize getImageId(Rc<Gfx::Image> image) {
        if (auto entry = mapping.lookup(reinterpret_cast<usize>(image._cell)))
            return entry.expect().v0;

        auto id = mapping.len() + 1;
        mapping.put(reinterpret_cast<usize>(image._cell), Tuple{id, image});
        return id;
    }
};

// 8.4.5 Graphics state parameter dictionaries
export struct GraphicalStateDict {
    f64 opacity;
    bool stroking = false;
};

struct StreamWriter {
    Io::StringWriter& _w;

    void str(Str s) {
        _w.append(s);
    }

    void num(f64 val, usize precision = 6) {
        Io::NumberBuffer buf;
        Io::NumberFormatter{.precision = precision}.formatFloat(buf, val);
        str(buf);
    }

    void uint(usize val) {
        Io::NumberBuffer buf;
        Io::NumberFormatter{}.formatUnsigned(buf, val);
        str(buf);
    }

    void hex4(u32 val) {
        Io::NumberBuffer buf;
        Io::NumberFormatter{.base = 16, .width = 4, .fillChar = '0'}.formatUnsigned(buf, val);
        str(buf);
    }

    void arg(f64 val, usize precision = 6) {
        Io::NumberBuffer buf;
        Io::NumberFormatter{.precision = precision}.formatFloat(buf, val);
        buf.pushBack(' ');
        str(buf);
    }

    void point(Math::Vec2f p) {
        Io::NumberBuffer buf;
        Io::NumberFormatter fmt{.precision = 2};
        fmt.formatFloat(buf, p.x);
        buf.pushBack(' ');
        fmt.formatFloat(buf, p.y);
        buf.pushBack(' ');
        str(buf);
    }

    void op(Str op) {
        str(op);
        str("\n"s);
    }
};

export struct Canvas : Gfx::Canvas {
    struct _Frame {
        Opt<Gfx::Color> fill = NONE;
        Opt<Gfx::Stroke> stroke = NONE;
        f64 opacity = 1.0;
        // From user space to the default PDF space, folded translations included.
        Math::Trans2f ctm = Math::Trans2f::identity();
        // Pure translations are folded into emitted coordinates instead of becoming "q ... cm ... Q".
        Math::Vec2f offset{};
        bool saved = false;
    };

    // What the content stream has actually set, saved and restored with q/Q.
    struct _GraphicState {
        Opt<Gfx::Color> rg = NONE;
        f64 ca = 1.0;
        void const* tfFace = nullptr;
        f64 tfSize = 0;

        Opt<Gfx::Color> RG = NONE;
        f64 CA = 1.0;
        Opt<f64> lineWidth = NONE;
        Opt<usize> lineCap = NONE;
        Opt<usize> lineJoin = NONE;
        Vec<f64> dashPattern = {};
        f64 dashOffset = 0;
    };

    Io::StringWriter& _out;
    Io::StringWriter _path;
    // In default PDF space, the unit the page's MediaBox is expressed in.
    Math::Vec2f _mediaBox{};
    Math::Vec2f _p{};
    Math::Vec2f _subpathStart{};

    FontManager& _fontManager;
    ImageManager& _imageManager;
    Vec<GraphicalStateDict>& _graphicalStates;

    Vec<_Frame> _stack;
    Vec<_GraphicState> _gstack;

    Canvas(Io::StringWriter& out, Math::Vec2f mediaBox, FontManager& fontManager, ImageManager& imageManager, Vec<GraphicalStateDict>& graphicalStates)
        : _out{out}, _mediaBox{mediaBox}, _fontManager{fontManager}, _imageManager{imageManager}, _graphicalStates(graphicalStates) {
        _stack.pushBack({});
        _gstack.pushBack({});
    }

    StreamWriter _e() {
        return {_out};
    }

    StreamWriter _pe() {
        return {_path};
    }

    // MARK: Graphics State ----------------------------------------------------

    void _ensureSaved() {
        auto& frame = last(_stack);
        if (_stack.len() == 1 or frame.saved)
            return;
        _e().op("q"s);
        frame.saved = true;
        _gstack.pushBack(last(_gstack));
    }

    bool _alphaInSync(f64 alpha, bool stroking = false) {
        auto& g = last(_gstack);
        return Math::epsilonEq(stroking ? g.CA : g.ca, alpha, 0.001);
    }

    bool _colorInSync(Gfx::Color color) {
        auto& g = last(_gstack);
        return g.rg and
               g.rg->red == color.red and
               g.rg->green == color.green and
               g.rg->blue == color.blue and
               _alphaInSync(color.alpha / 255.0 * last(_stack).opacity);
    }

    void _syncAlpha(f64 alpha, bool stroking = false) {
        if (_alphaInSync(alpha, stroking))
            return;

        usize gsIndex = _graphicalStates.len();
        for (usize i = 0; i < _graphicalStates.len(); ++i) {
            auto const& gs = _graphicalStates[i];
            if (gs.stroking == stroking and Math::epsilonEq(gs.opacity, alpha, 0.001)) {
                gsIndex = i;
                break;
            }
        }

        if (gsIndex == _graphicalStates.len())
            _graphicalStates.pushBack(GraphicalStateDict{alpha, stroking});

        auto e = _e();
        e.str("/GS"s);
        e.uint(gsIndex);
        e.op(" gs"s);
        (stroking ? last(_gstack).CA : last(_gstack).ca) = alpha;
    }

    void _syncColor(Gfx::Color color) {
        _syncColor(color, last(_stack).opacity);
    }

    void _syncColor(Gfx::Color color, f64 opacity) {
        auto& g = last(_gstack);
        if (not g.rg or
            g.rg->red != color.red or
            g.rg->green != color.green or
            g.rg->blue != color.blue) {
            auto e = _e();
            e.arg(color.red / 255.0, 3);
            e.arg(color.green / 255.0, 3);
            e.arg(color.blue / 255.0, 3);
            e.op("rg"s);
            g.rg = Some(color);
        }
        _syncAlpha(color.alpha / 255.0 * opacity);
    }

    void _syncFill() {
        auto& frame = last(_stack);
        if (frame.fill)
            _syncColor(*frame.fill);
        else
            _syncAlpha(frame.opacity);
    }

    void _syncStroke(Gfx::Stroke const& stroke) {
        auto color = stroke.fill.expect<Gfx::Color>();
        auto e = _e();
        auto& g = last(_gstack);

        if (not g.RG or
            g.RG->red != color.red or
            g.RG->green != color.green or
            g.RG->blue != color.blue) {
            e.arg(color.red / 255.0, 3);
            e.arg(color.green / 255.0, 3);
            e.arg(color.blue / 255.0, 3);
            e.op("RG"s);
            g.RG = Some(color);
        }

        if (g.lineWidth != stroke.width) {
            e.arg(stroke.width, 2);
            e.op("w"s);
            g.lineWidth = Some(stroke.width);
        }

        usize cap = 0;
        if (stroke.cap == Gfx::ROUND_CAP)
            cap = 1;
        else if (stroke.cap == Gfx::SQUARE_CAP)
            cap = 2;

        if (g.lineCap != cap) {
            e.uint(cap);
            e.op(" J"s);
            g.lineCap = Some(cap);
        }

        usize join = 0;
        if (stroke.join == Gfx::ROUND_JOIN)
            join = 1;
        else if (stroke.join == Gfx::BEVEL_JOIN)
            join = 2;

        if (g.lineJoin != join) {
            e.uint(join);
            e.op(" j"s);
            g.lineJoin = Some(join);
        }

        if (g.dashPattern != stroke.dashPattern or g.dashOffset != stroke.dashOffset) {
            e.str("["s);
            for (auto [dash, i] : iter(stroke.dashPattern) | Index()) {
                if (i)
                    e.str(" "s);
                e.num(dash, 2);
            }
            e.str("] "s);
            e.arg(stroke.dashOffset, 2);
            e.op("d"s);
            g.dashPattern = stroke.dashPattern;
            g.dashOffset = stroke.dashOffset;
        }

        if (stroke.align != Gfx::CENTER_ALIGN)
            logDebugIf(debugCanvas, "pdf: only centered strokes are supported");

        _syncAlpha(color.alpha / 255.0 * last(_stack).opacity, true);
    }

    // MARK: Context Operations ------------------------------------------------

    void push() override {
        auto frame = last(_stack);
        frame.saved = false;
        _stack.pushBack(frame);
    }

    void pop() override {
        if (_stack.len() <= 1)
            return;

        if (last(_stack).saved) {
            _e().op("Q"s);
            _gstack.popBack();
        }
        _stack.popBack();
    }

    void fillStyle(Gfx::Fill fill) override {
        last(_stack).fill = Some(fill.expect<Gfx::Color>());
    }

    void strokeStyle(Gfx::Stroke stroke) override {
        last(_stack).stroke = Some(std::move(stroke));
    }

    void opacity(f64 opacity) override {
        last(_stack).opacity = opacity;
    }

    void transform(Math::Trans2f trans) override {
        if (trans.isIdentity())
            return;

        auto& frame = last(_stack);
        frame.ctm = trans.multiply(frame.ctm);

        if (trans.xx == 1 and trans.xy == 0 and trans.yx == 0 and trans.yy == 1) {
            frame.offset = frame.offset + Math::Vec2f{trans.ox, trans.oy};
            return;
        }

        _ensureSaved();
        auto e = _e();
        e.arg(trans.xx);
        e.arg(trans.xy);
        e.arg(trans.yx);
        e.arg(trans.yy);
        e.arg(trans.ox + frame.offset.x);
        e.arg(trans.oy + frame.offset.y);
        e.op("cm"s);
        frame.offset = {};
    }

    // MARK: Path Operations ---------------------------------------------------

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

    Math::Vec2f _device(Math::Vec2f p) {
        return p + last(_stack).offset;
    }

    // Rounds the corners rather than the size, so edges land where m/l would have put them.
    void _emitRect(StreamWriter e, Math::Vec2f start, Math::Vec2f end) {
        auto round2 = [](f64 v) {
            return Math::round(v * 100) / 100;
        };
        auto x = round2(start.x);
        auto y = round2(start.y);
        e.arg(x, 2);
        e.arg(y, 2);
        e.arg(round2(end.x) - x, 2);
        e.arg(round2(end.y) - y, 2);
        e.op("re"s);
    }

    // The path outlives painting, like on the other canvases: SVG shapes fill then stroke it.
    void _paintPath() {
        _out.append(_path.str());
    }

    void beginPath() override {
        _path.clear();
    }

    void closePath() override {
        _pe().op("h"s);
        _p = _subpathStart;
    }

    void moveTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
        auto e = _pe();
        e.point(_device(_mapPointAndUpdate(p, options)));
        e.op("m"s);
        _subpathStart = _p;
    }

    void lineTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
        auto e = _pe();
        e.point(_device(_mapPointAndUpdate(p, options)));
        e.op("l"s);
    }

    void hlineTo(f64 x, Flags<Math::Path::Option> options) override {
        lineTo({options & Math::Path::RELATIVE ? _p.x + x : x, _p.y}, {});
    }

    void vlineTo(f64 y, Flags<Math::Path::Option> options) override {
        lineTo({_p.x, options & Math::Path::RELATIVE ? _p.y + y : y}, {});
    }

    void cubicTo(Math::Vec2f cp1, Math::Vec2f cp2, Math::Vec2f p, Flags<Math::Path::Option> options) override {
        cp1 = _mapPoint(cp1, options);
        cp2 = _mapPoint(cp2, options);
        p = _mapPointAndUpdate(p, options);
        auto e = _pe();
        e.point(_device(cp1));
        e.point(_device(cp2));
        e.point(_device(p));
        e.op("c"s);
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

    void rect(Math::Rectf rect, Math::Radiif radii) override {
        if (not radii.zero()) {
            Gfx::Canvas::rect(rect, radii);
            return;
        }

        if (Math::epsilonEq(min(rect.width, rect.height), 0.0, 0.001))
            return;

        _emitRect(_pe(), _device(rect.topStart()), _device(rect.bottomEnd()));
        _p = _subpathStart = rect.topStart();
    }

    void ellipse(Math::Ellipsef ellipse) override {
        Math::Path p;
        p.ellipse(ellipse);
        path(p);
    }

    void arc(Math::Arcf arc) override {
        Math::Path p;
        p.arc(arc);
        path(p);
    }

    void path(Math::Path const& path) override {
        for (auto& contour : path.iterContours()) {
            moveTo(contour[0], {});
            for (auto& vert : next(contour))
                lineTo(vert, {});
            if (contour.close)
                closePath();
        }
    }

    void fill(Gfx::FillRule rule) override {
        _syncFill();
        _paintPath();
        _e().op(rule == Gfx::FillRule::NONZERO ? "f"s : "f*"s);
    }

    void fill(Rc<Gfx::Prose> p) override {
        auto& prose = *p;
        auto offset = last(_stack).offset;
        auto e = _e();

        e.op("BT"s);

        for (usize i = 0; i < prose._lines.len(); ++i) {
            auto const& line = prose._lines[i];

            if (not line.blocks(prose))
                continue;

            auto lineStartPos = first(line.blocks(prose)).pos.cast<f64>();
            auto lineBaseline = line.baseline.cast<f64>();

            e.str("1 0 0 -1 "s);
            e.arg(lineStartPos + offset.x);
            e.arg(lineBaseline + offset.y);
            e.op("Tm"s);

            auto prevEndPos = lineStartPos;
            bool inArray = false;

            auto ensureState = [&](Gfx::SpanStyle const& style) {
                auto color = style.color ? *style.color : last(_stack).fill.unwrapOr(Gfx::BLACK);
                auto& g = last(_gstack);
                bool colorChanged = not _colorInSync(color);
                bool fontChanged =
                    g.tfFace != style.font.fontface._cell or
                    g.tfSize != style.font.fontsize;

                if (inArray and (colorChanged or fontChanged)) {
                    e.op(">] TJ"s);
                    inArray = false;
                }

                if (colorChanged)
                    _syncColor(color);

                if (fontChanged) {
                    e.str("/F"s);
                    e.uint(_fontManager.getFontId(style.font.fontface));
                    e.str(" "s);
                    e.arg(style.font.fontsize);
                    e.op("Tf"s);
                    g.tfFace = style.font.fontface._cell;
                    g.tfSize = style.font.fontsize;
                }

                if (not inArray) {
                    e.str("[<"s);
                    inArray = true;
                }
            };

            for (auto& block : line.blocks(prose)) {
                for (auto& cell : block.cells(prose)) {
                    if (cell.type() == Gfx::Prose::CellType::STRUT)
                        continue;

                    auto const& style = cell.style();
                    ensureState(style);
                    f64 fontSize = style.font.fontsize;

                    if (cell.type() == Gfx::Prose::CellType::SPACER) {
                        e.str(">"s);
                        e.num(-(cell.adv.cast<f64>() * (1000.0 / fontSize)));
                        e.str("<"s);
                        prevEndPos = prevEndPos + cell.adv.cast<f64>();
                        continue;
                    }

                    auto glyphAdvance = style.font.advance(cell.glyph);
                    auto nextEndPosWithoutKern = prevEndPos + glyphAdvance;
                    auto nextDesiredEndPos = (block.pos + cell.pos + cell.adv).cast<f64>();

                    auto kernDiff = nextEndPosWithoutKern - nextDesiredEndPos;

                    if (not Math::epsilonEq<f64>(kernDiff, 0, 0.01)) {
                        e.str(">"s);
                        e.num(kernDiff * (1000.0 / fontSize));
                        e.str("<"s);
                    }

                    if (cell.newline(prose)) {
                        Array<Rune, 1> space = {' '};
                        e.hex4(_fontManager.code(style.font.fontface, style.font.glyph(' '), space));
                    } else {
                        e.hex4(_fontManager.code(style.font.fontface, cell.glyph, cell.runes(prose)));
                    }

                    prevEndPos = prevEndPos + glyphAdvance - kernDiff;
                }
            }

            if (inArray)
                e.op(">] TJ"s);
        }

        e.op("ET"s);
    }

    void fill(Gfx::Fill f, Gfx::FillRule rule) override {
        auto saved = last(_stack).fill;
        fillStyle(f);
        fill(rule);
        last(_stack).fill = saved;
    }

    void stroke() override {
        static Gfx::Stroke const DEFAULT_STROKE = {};
        auto const& frame = last(_stack);
        _syncStroke(frame.stroke ? *frame.stroke : DEFAULT_STROKE);
        _paintPath();
        _e().op("S"s);
    }

    void clip(Gfx::FillRule rule) override {
        _ensureSaved();
        _paintPath();
        _e().op(rule == Gfx::FillRule::EVENODD ? "W* n"s : "W n"s);
    }

    void apply(Gfx::Filter) override {
        logDebugIf(debugCanvas, "pdf: apply() operation not implemented");
    };

    // MARK: Direct Painting ---------------------------------------------------
    // clear() and plot() paint their own shape, the current path must survive them.

    void _directPolygon(Slice<Math::Vec2f> points, bool close) {
        auto e = _e();
        for (auto [p, i] : iter(points) | Index()) {
            e.point(_device(p));
            e.op(i ? "l"s : "m"s);
        }
        if (close)
            e.op("h"s);
    }

    void _directFill(Slice<Math::Vec2f> points, Gfx::Color color) {
        _syncColor(color, 1.0);
        _directPolygon(points, true);
        _e().op("f"s);
    }

    void _directStroke(Slice<Math::Vec2f> points, bool close, Gfx::Color color) {
        _syncStroke({.fill = color, .width = 1, .cap = Gfx::SQUARE_CAP});
        _directPolygon(points, close);
        _e().op("S"s);
    }

    static Array<Math::Vec2f, 4> _corners(Math::Rectf r) {
        return {r.topStart(), r.topEnd(), r.bottomEnd(), r.bottomStart()};
    }

    // MARK: Clear Operations --------------------------------------------------

    void clear(Gfx::Color color) override {
        auto toUser = last(_stack).ctm.inverse();
        auto page = _corners({0, 0, _mediaBox.x, _mediaBox.y});
        for (auto& p : page)
            p = toUser.apply(p);
        _directFill(page, color);
    }

    void clear(Math::Recti rect, Gfx::Color color) override {
        _directFill(_corners(rect.cast<f64>()), color);
    }

    // MARK: Plot Operations ---------------------------------------------------

    void plot(Math::Vec2i point, Gfx::Color color) override {
        _directFill(_corners({point.cast<f64>(), {1, 1}}), color);
    }

    void plot(Math::Edgei edge, Gfx::Color color) override {
        Array<Math::Vec2f, 2> points = {
            edge.start.cast<f64>() + 0.5,
            edge.end.cast<f64>() + 0.5,
        };
        _directStroke(points, false, color);
    }

    void plot(Math::Recti rect, Gfx::Color color) override {
        auto r = rect.cast<f64>();
        _directStroke(_corners({r.xy + 0.5, r.wh - 1}), true, color);
    }

    // MARK: Blit Operations ---------------------------------------------------

    void blit(Math::Recti src, Math::Recti dest, Rc<Gfx::Image> image) override {
        if (src.width <= 0 or src.height <= 0)
            return;

        auto srcf = src.cast<f64>();
        auto destf = dest.cast<f64>();
        auto size = image->bound().size().cast<f64>();
        Math::Vec2f scale = {destf.width / srcf.width, destf.height / srcf.height};
        Math::Rectf placed = {
            destf.x - srcf.x * scale.x,
            destf.y - srcf.y * scale.y,
            size.x * scale.x,
            size.y * scale.y,
        };

        push();

        if (src != image->bound()) {
            _ensureSaved();
            auto e = _e();
            _emitRect(e, _device(destf.topStart()), _device(destf.bottomEnd()));
            e.op("W n"s);
        }

        transform({placed.width, 0, 0, -placed.height, placed.x, placed.y + placed.height});
        _syncAlpha(last(_stack).opacity);

        auto e = _e();
        e.str("/Im"s);
        e.uint(_imageManager.getImageId(image));
        e.op(" Do"s);

        pop();
    }
};

} // namespace Karm::Pdf
