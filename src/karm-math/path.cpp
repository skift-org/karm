module;

#include <karm-core/macros.h>

export module Karm.Math:path;

import Karm.Core;

import :arc;
import :curve;
import :edge;
import :ellipse;
import :radii;
import :rect;
import :vec;
import :trans;

namespace Karm::Math {

export struct Path {
    enum struct Code {
        NOP,
        CLEAR,
        CLOSE,
        MOVE_TO,
        LINE_TO,
        HLINE_TO,
        VLINE_TO,
        CUBIC_TO,
        QUAD_TO,
        ARC_TO,
    };

    using enum Code;

    enum struct Option {
        LARGE = 1 << 1,
        SWEEP = 1 << 2,
        SMOOTH = 1 << 3,
        RELATIVE = 1 << 4,
    };

    using enum Option;

    struct Op {
        Code code{};
        Flags<Option> options{};

        Vec2f radii{};
        f64 angle{};
        Vec2f cp1{};
        Vec2f cp2{};
        Vec2f p{};

        Op(Code code, Flags<Option> options = {})
            : code(code), options(options) {}

        Op(Code code, Vec2f p, Flags<Option> options = {})
            : code(code), options(options), p(p) {}

        Op(Code code, Vec2f cp2, Vec2f p, Flags<Option> options = {})
            : code(code), options(options), cp2(cp2), p(p) {}

        Op(Code code, Vec2f cp1, Vec2f cp2, Vec2f p, Flags<Option> options = {})
            : code(code), options(options), cp1(cp1), cp2(cp2), p(p) {}

        Op(Code code, Vec2f radii, f64 angle, Vec2f p, Flags<Option> options = {})
            : code(code), options(options), radii(radii), angle(angle), p(p) {}

        void repr(Io::Emit& e) const {
            e("(Op");
            switch (code) {
            case NOP:
                e(" NOP");
                break;
            case CLEAR:
                e(" CLEAR");
                break;
            case CLOSE:
                e(" CLOSE");
                break;
            case MOVE_TO:
                e(" MOVE_TO p:{}", p);
                break;
            case LINE_TO:
                e(" LINE_TO {}", p);
                break;
            case HLINE_TO:
                e(" HLINE_TO {}", p);
                break;
            case VLINE_TO:
                e(" VLINE_TO {}", p);
                break;
            case CUBIC_TO:
                e(" CUBIC_TO cp1: {} cp2: {} p: {}", cp1, cp2, p);
                break;
            case QUAD_TO:
                e(" QUAD_TO cp: {} p: {}", cp2, p);
                break;
            case ARC_TO:
                e(" ARC_TO radii: {} angle: {} p: {}", radii, angle, p);
                break;
            }

            if (options & LARGE)
                e(" LARGE");

            if (options & RELATIVE)
                e(" RELATIVE");

            if (options & SWEEP)
                e(" SWEEP");

            if (options & SMOOTH)
                e(" SMOOTH");

            e(")");
        }
    };

    struct Contour {
        usize start{};
        usize end{};
        bool close{};
    };

    Vec<Contour> _contours{};
    Vec<Vec2f> _verts{};

    Vec2f _lastCp;
    Vec2f _lastP;

    struct _Contour : Slice<Vec2f> {
        bool close;

        _Contour(Slice<Vec2f> slice, bool close)
            : Slice<Vec2f>(slice), close(close) {}
    };

    auto iterContours() const {
        return Iter([&, i = 0uz] mutable -> Opt<_Contour> {
            if (i >= _contours.len()) {
                return NONE;
            }

            i++;

            return _Contour{
                sub(_verts, _contours[i - 1].start, _contours[i - 1].end),
                _contours[i - 1].close,
            };
        });
    }

    Opt<Rectf> _bound;

    Rectf bound() {
        if (isEmpty(_verts))
            return {};

        if (_bound)
            return *_bound;

        Rectf rect = {_verts[0], 0};
        for (auto& p : _verts)
            rect = rect.mergeWith(p);
        _bound = rect;
        return rect;
    }

    // MARK: Flattening --------------------------------------------------------

    void _flattenClose() {
        if (_contours.len() > 1) {
            auto end = _verts[last(_contours).end - 1];
            auto start = _verts[last(_contours).start];

            // NOTE: remove last edge if it is manually closing the path, since 'z' should be the one closing
            if (epsilonEq(start, end, 0.001)) {
                _verts.popBack();
                last(_contours).end--;
            }
        }

        last(_contours).close = true;
    }

    void _flattenLineTo(Vec2f p) {
        if (not _contours.len()) {
            // moveTo must be called before lineTo
            return;
        }

        if (last(_contours).start != last(_contours).end and
            epsilonEq(last(_verts), p)) {
            return;
        }

        _verts.pushBack(p);
        last(_contours).end++;
    }

    void _flattenCurveTo(Curvef curve, isize depth = 0) {
        if (depth > 16)
            return;

        if (curve.degenerated())
            return;

        if (curve.straightish(0.25)) {
            _flattenLineTo(curve.d);
            return;
        }

        auto [left, right] = curve.split();
        _flattenCurveTo(left, depth + 1);
        _flattenCurveTo(right, depth + 1);
    }

    void _flattenArcTo(Vec2f start, Vec2f radii, f64 angle, Flags<Option> options, Vec2f point) {
        // Ported from canvg (https://github.com/canvg/canvg)
        f64 x1 = start.x;
        f64 y1 = start.y;
        f64 x2 = point.x;
        f64 y2 = point.y;

        f64 dx = x1 - x2;
        f64 dy = y1 - y2;
        f64 d = sqrt(dx * dx + dy * dy);

        if (d < 1e-6f or radii.x < 1e-6f or radii.y < 1e-6f) {
            // The arc degenerates to a line
            _flattenLineTo(point);
            return;
        }

        f64 rotx = angle / 180.0 * PI; // x rotation angle
        f64 sinrx = sin(rotx);
        f64 cosrx = cos(rotx);

        // Convert to center point parameterization.
        // http://www.w3.org/TR/SVG11/implnote.html#ArcImplementationNotes

        // 1) Compute x1', y1'
        f64 x1p = cosrx * dx / 2.0 + sinrx * dy / 2.0;
        f64 y1p = -sinrx * dx / 2.0 + cosrx * dy / 2.0;

        d = pow2(x1p) / pow2(radii.x) + pow2(y1p) / pow2(radii.y);

        if (d > 1) {
            d = sqrt(d);
            radii.x *= d;
            radii.y *= d;
        }

        // 2) Compute cx', cy'
        f64 sa = pow2(radii.x) * pow2(radii.y) -
                 pow2(radii.x) * pow2(y1p) -
                 pow2(radii.y) * pow2(x1p);

        f64 sb = pow2(radii.x) * pow2(y1p) +
                 pow2(radii.y) * pow2(x1p);

        if (sa < 0.0)
            sa = 0.0;

        f64 s = 0.0;

        if (sb > 0.0)
            s = sqrt(sa / sb);

        bool fa = options & LARGE;
        bool fs = options & SWEEP;

        if (fa == fs) {
            s = -s;
        }

        f64 cxp = s * radii.x * y1p / radii.y;
        f64 cyp = s * -radii.y * x1p / radii.x;

        // 3) Compute cx,cy from cx',cy'
        f64 cx = cosrx * cxp - sinrx * cyp + (x1 + x2) / 2.0;
        f64 cy = sinrx * cxp + cosrx * cyp + (y1 + y2) / 2.0;

        // 4) Calculate theta1, and delta theta.
        Vec2f u = {(x1p - cxp) / radii.x, (y1p - cyp) / radii.y};
        Vec2f v = {(-x1p - cxp) / radii.x, (-y1p - cyp) / radii.y};

        f64 a1 = Vec2f(1, 0).angleWith(u); // Initial angle
        f64 da = u.angleWith(v);

        if (not fs and da > 0) {
            da -= 2 * PI;
        } else if (fs and da < 0) {
            da += 2 * PI;
        }

        // Approximate the arc using cubic spline segments.
        Trans2f t{cosrx, sinrx, -sinrx, cosrx, cx, cy};

        // Split arc into max 90 degree segments.
        // The loop assumes an Iter per end point (including start and end), this +1.
        isize ndivs = (isize)(abs(da) / (PI * 0.5) + 1.0);
        f64 hda = (da / (f64)ndivs) / 2.0;
        f64 kappa = abs(4.0 / 3.0 * (1.0 - cos(hda)) / sin(hda));

        if (da < 0.0) {
            kappa = -kappa;
        }

        Vec2f current = {};
        Vec2f ptan = {};

        for (isize i = 0; i <= ndivs; i++) {
            f64 a = a1 + da * (i / (f64)ndivs);

            dx = cos(a);
            dy = sin(a);

            Vec2f p = t.apply(Vec2f{dx * radii.x, dy * radii.y});
            Vec2f tan = t.applyVector({-dy * radii.x * kappa, dx * radii.y * kappa});

            if (i > 0)
                _flattenCurveTo(Curvef::cubic(current, current + ptan, p - tan, p));

            current = p;
            ptan = tan;
        }
    }

    // MARK: Operations --------------------------------------------------------

    void evalOp(Op op) {
        if (_contours.len() > 0 and
            last(_contours).close and
            op.code != MOVE_TO and
            op.code != CLEAR) {
            // can't evalOp on closed path
            return;
        }

        if (op.options & RELATIVE) {
            op.cp1 = _lastP + op.cp1;
            op.cp2 = _lastP + op.cp2;
            op.p = _lastP + op.p;
            op.options.unset(RELATIVE);
        }

        switch (op.code) {
        case CLEAR:
            _lastCp = {};
            _lastP = {};
            _contours.clear();
            _verts.clear();
            break;

        case CLOSE:
            op.p = _verts[last(_contours).start];
            _flattenClose();
            _lastCp = op.p;
            break;

        case MOVE_TO:
            _contours.pushBack({_verts.len(), _verts.len(), false});
            _flattenLineTo(op.p);
            _lastCp = op.p;
            break;

        case LINE_TO:
            _flattenLineTo(op.p);
            _lastCp = op.p;
            break;

        case HLINE_TO:
            op.p = {op.p.x, _lastP.y};
            _flattenLineTo(op.p);
            _lastCp = op.p;
            break;

        case VLINE_TO:
            op.p = {_lastP.x, op.p.y};
            _flattenLineTo(op.p);
            _lastCp = op.p;
            break;

        case CUBIC_TO:
            if (op.options & SMOOTH)
                op.cp1 = _lastP * 2 - _lastCp;
            _flattenCurveTo(Curvef::cubic(_lastP, op.cp1, op.cp2, op.p));
            _lastCp = op.cp2;
            break;

        case QUAD_TO:
            if (op.options & SMOOTH)
                op.cp2 = _lastP * 2 - _lastCp;
            _flattenCurveTo(Curvef::quadratic(_lastP, op.cp2, op.p));
            _lastCp = op.cp2;
            break;

        case ARC_TO:
            _flattenArcTo(_lastP, op.radii, op.angle, op.options, op.p);
            _lastCp = op.p;
            break;

        default:
            panic("unknown path opcode");
        }

        _lastP = op.p;
    }

    void clear() {
        evalOp(CLEAR);
    }

    void close() {
        evalOp(CLOSE);
    }

    void moveTo(Vec2f p, Flags<Option> options = {}) {
        evalOp({MOVE_TO, p, options});
    }

    void lineTo(Vec2f p, Flags<Option> options = {}) {
        evalOp({LINE_TO, p, options});
    }

    void hlineTo(f64 x, Flags<Option> options = {}) {
        evalOp({HLINE_TO, {x, 0}, options});
    }

    void vlineTo(f64 y, Flags<Option> options = {}) {
        evalOp({VLINE_TO, {0, y}, options});
    }

    void cubicTo(Vec2f cp1, Vec2f cp2, Vec2f p, Flags<Option> options = {}) {
        evalOp({CUBIC_TO, cp1, cp2, p, options});
    }

    void smoothCubicTo(Vec2f cp2, Vec2f p, Flags<Option> options = {}) {
        evalOp({CUBIC_TO, {}, cp2, p, options | SMOOTH});
    }

    void quadTo(Vec2f cp, Vec2f p, Flags<Option> options = {}) {
        evalOp({QUAD_TO, cp, p, options});
    }

    void smoothQuadTo(Vec2f p, Flags<Option> options = {}) {
        evalOp({QUAD_TO, {}, p, options | SMOOTH});
    }

    void arcTo(Vec2f radii, f64 angle, Vec2f p, Flags<Option> options = {}) {
        evalOp({ARC_TO, radii, angle, p, options});
    }

    // MARK: Shapes ------------------------------------------------------------

    void line(Edgef edge) {
        moveTo(edge.start);
        lineTo(edge.end);
    }

    void curve(Curvef curve) {
        moveTo(curve.a);
        cubicTo(curve.b, curve.c, curve.d);
    }

    void rect(Rectf rect, Radiif radii = 0) {
        if (epsilonEq(min(rect.width, rect.height), 0.0, 0.001))
            return;

        if (radii.zero()) {
            moveTo(rect.topStart());
            lineTo(rect.topEnd());
            lineTo(rect.bottomEnd());
            lineTo(rect.bottomStart());
            close();
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

            moveTo({rect.x + radii.b, rect.y});

            // Top end edge
            lineTo({rect.x + rect.width - radii.c, rect.y});
            cubicTo(
                {rect.x + rect.width - cpc, rect.y},
                {rect.x + rect.width, rect.y + cpd},
                {rect.x + rect.width, rect.y + radii.d}
            );

            // Bottom end edge
            lineTo({rect.x + rect.width, rect.y + rect.height - radii.e});
            cubicTo(
                {rect.x + rect.width, rect.y + rect.height - cpe},
                {rect.x + rect.width - cpf, rect.y + rect.height},
                {rect.x + rect.width - radii.f, rect.y + rect.height}
            );

            // Bottom start edge
            lineTo({rect.x + radii.g, rect.y + rect.height});
            cubicTo(
                {rect.x + cpg, rect.y + rect.height},
                {rect.x, rect.y + rect.height - cph},
                {rect.x, rect.y + rect.height - radii.h}
            );

            // Top start edge
            lineTo({rect.x, rect.y + radii.a});
            cubicTo(
                {rect.x, rect.y + cpa},
                {rect.x + cpb, rect.y},
                {rect.x + radii.b, rect.y}
            );

            close();
        }
    }

    void ellipse(Ellipsef ellipse) {
        auto bound = ellipse.bound();
        moveTo(bound.topCenter());
        arcTo(ellipse.radii, 0, bound.bottomCenter(), SWEEP);
        arcTo(ellipse.radii, 0, bound.topCenter(), SWEEP);
        close();
    }

    void arc(Arcf arc) {
        moveTo(arc.eval(0.0));
        for (auto t = 0.0; t < 1.0; t += 0.1) {
            auto p = arc.eval(t);
            lineTo(p);
        }
    }

    void path(Path const& path) {
        for (auto contour : path.iterContours()) {
            if (contour.len() == 0)
                panic("it is not possible to have an empty contour at this point");

            moveTo(first(contour));
            for (auto v : next(contour, 1)) {
                lineTo(v);
            }

            if (contour.close)
                close();
        }
    }

    // MARK: Transform ---------------------------------------------------------

    void offset(Vec2f offset) {
        for (auto& v : _verts)
            v = v + offset;
    }

    // MARK: Svg ---------------------------------------------------------------

    Opt<Vec2f> _nextVec2f(Io::SScan& s) {
        return Vec2f{
            try$(Io::atof(s)),
            try$(Io::atof(s)),
        };
    }

    static Opt<Op> parseOp(Io::SScan& s, Rune opcode) {
        Flags<Option> options{};

        options.set(RELATIVE, isAsciiLower(opcode));
        opcode = toAsciiLower(opcode);

        auto nextSep = [&] {
            s.skip(Re::optSeparator(','_re));
        };

        auto nextCoord = [&] -> Opt<f64> {
            nextSep();
            return try$(Io::atof(s));
        };

        auto nextCoordPair = [&] -> Opt<Vec2f> {
            auto r = Vec2f{try$(nextCoord()), try$(nextCoord())};
            nextSep();
            return r;
        };

        switch (opcode) {
        case 'm': // move to
            return Op{MOVE_TO, try$(nextCoordPair()), options};

        case 'z': // close
            return Op{CLOSE, options};

        case 'l': // line to
            return Op{LINE_TO, try$(nextCoordPair()), options};

        case 'h': // horizontal line to
            return Op{HLINE_TO, try$(nextCoord()), options};

        case 'v': // vertical line to
            return Op{VLINE_TO, try$(nextCoord()), options};

        case 'c': // cubic to
            return Op{CUBIC_TO, try$(nextCoordPair()), try$(nextCoordPair()), try$(nextCoordPair()), options};

        case 's': // smooth cubic to
            return Op{CUBIC_TO, {}, try$(nextCoordPair()), try$(nextCoordPair()), options | SMOOTH};

        case 'q': // quad to
            return Op{QUAD_TO, try$(nextCoordPair()), try$(nextCoordPair()), options};

        case 't': // smooth quad to
            return Op{QUAD_TO, {}, try$(nextCoordPair()), options | SMOOTH};

        case 'a': // arc to
        {
            auto radii = try$(nextCoordPair());
            auto angle = try$(nextCoord());

            nextSep();
            if (s.next() == '1')
                options |= LARGE;
            nextSep();
            if (s.next() == '1')
                options |= SWEEP;

            auto p = try$(nextCoordPair());
            return Op{ARC_TO, radii, angle, p, options};
        }

        default:
            return NONE;
        }
    }

    bool evalSvg(Str svg) {
        Io::SScan s{svg};

        Rune opcode{};
        while ((opcode = s.next())) {
            s.skip(Re::zeroOrMore(Re::space()));

            do {
                auto maybeOp = parseOp(s, opcode);

                if (not maybeOp) {
                    return false;
                }

                evalOp(maybeOp.unwrap());
                s.skip(Re::zeroOrMore(Re::space()));

                opcode = opcode == 'M' ? 'L' : opcode;
                opcode = opcode == 'm' ? 'l' : opcode;
            } while (not s.ended() and not isAsciiAlpha(s.peek()));
        }

        return true;
    }

    static Path fromSvg(Str svg) {
        Path p;
        // eval path might fail, in this case you get an empty path
        (void)p.evalSvg(svg);
        return p;
    }

    // MARK: Format

    void repr(Io::Emit& e) const {
        e("(path");
        e.indentNewline();
        for (auto& contour : iterContours()) {
            e("(contour {})\n", contour);
        }
        e(")");
    }
};

} // namespace Karm::Math
