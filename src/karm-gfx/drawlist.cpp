export module Karm.Gfx:drawlist;

import Karm.Core;
import Karm.Math;

import :fill;
import :filters;
import :font;
import :stroke;
import :prose;

namespace Karm::Gfx {

struct _Push {
    void repr(Io::Emit& e) const {
        e("(Push)");
    }
};

struct _Pop {
    void repr(Io::Emit& e) const {
        e("(Pop)");
    }
};

struct _FillStyle {
    Fill style;

    void repr(Io::Emit& e) const {
        e("(FillStyle style:{})", style);
    }
};

struct _StrokeStyle {
    Stroke style;

    void repr(Io::Emit& e) const {
        e("(StrokeStyle style:{})", style);
    }
};

struct _Opacity {
    f64 opacity;

    void repr(Io::Emit& e) const {
        e("(Opacity opacity:{})", opacity);
    }
};

struct _Transform {
    Math::Trans2f trans;

    void repr(Io::Emit& e) const {
        e("(Transform trans:{})", trans);
    }
};

struct _Translate {
    Math::Vec2f pos;

    void repr(Io::Emit& e) const {
        e("(Translate pos:{})", pos);
    }
};

struct _BeginPath {
    void repr(Io::Emit& e) const {
        e("(BeginPath)");
    }
};

struct _ClosePath {
    void repr(Io::Emit& e) const {
        e("(ClosePath)");
    }
};

struct _MoveTo {
    Math::Vec2f p;
    Flags<Math::Path::Option> options;

    void repr(Io::Emit& e) const {
        e("(MoveTo p:{} options:{})", p, options);
    }
};

struct _LineTo {
    Math::Vec2f p;
    Flags<Math::Path::Option> options;

    void repr(Io::Emit& e) const {
        e("(LineTo p:{} options:{})", p, options);
    }
};

struct _HLineTo {
    f64 x;
    Flags<Math::Path::Option> options;

    void repr(Io::Emit& e) const {
        e("(HLineTo x:{} options:{})", x, options);
    }
};

struct _VLineTo {
    f64 y;
    Flags<Math::Path::Option> options;

    void repr(Io::Emit& e) const {
        e("(VLineTo y:{} options:{})", y, options);
    }
};

struct _CubicTo {
    Math::Vec2f cp1;
    Math::Vec2f cp2;
    Math::Vec2f p;
    Flags<Math::Path::Option> options;

    void repr(Io::Emit& e) const {
        e("(CubicTo cp1:{} cp2:{} p:{} options:{})", cp1, cp2, p, options);
    }
};

struct _QuadTo {
    Math::Vec2f cp;
    Math::Vec2f p;
    Flags<Math::Path::Option> options;

    void repr(Io::Emit& e) const {
        e("(QuadTo cp:{} p:{} options:{})", cp, p, options);
    }
};

struct _ArcTo {
    Math::Vec2f radius;
    f64 angle;
    Math::Vec2f p;
    Flags<Math::Path::Option> options;

    void repr(Io::Emit& e) const {
        e("(ArcTo radius:{} angle:{} p:{} options:{})", radius, angle, p, options);
    }
};

struct _Line {
    Math::Edgef line;

    void repr(Io::Emit& e) const {
        e("(Line line:{})", line);
    }
};

struct _Curve {
    Math::Curvef curve;

    void repr(Io::Emit& e) const {
        e("(Curve curve:{})", curve);
    }
};

struct _Ellipse {
    Math::Ellipsef ellipse;

    void repr(Io::Emit& e) const {
        e("(Ellipse ellipse:{})", ellipse);
    }
};

struct _Arc {
    Math::Arcf arc;

    void repr(Io::Emit& e) const {
        e("(Arc arc:{})", arc);
    }
};

struct _Path {
    Rc<Math::Path> path;

    void repr(Io::Emit& e) const {
        e("(Path path:{})", path);
    }
};

struct _Fill {
    void repr(Io::Emit& e) const {
        e("(Fill)");
    }
};

struct _Stroke {
    void repr(Io::Emit& e) const {
        e("(Stroke)");
    }
};

struct _Clip {
    FillRule rule;

    void repr(Io::Emit& e) const {
        e("(Clip rule:{})", rule);
    }
};

struct _Apply {
    Rc<Filter> filter;

    void repr(Io::Emit& e) const {
        e("(Apply filter:{})", filter);
    }
};

struct _Clear {
    Opt<Math::Recti> rect;
    Color color;

    void repr(Io::Emit& e) const {
        e("(Clear rect:{} color:{})", rect, color);
    }
};

struct _Blit {
    Math::Recti src;
    Math::Recti dest;
    Rc<Surface> surface;

    void repr(Io::Emit& e) const {
        e("(Blit src:{} dest:{} surface:{})", src, dest, surface);
    }
};

export using DrawCommand = Union<
    _Push,
    _Pop,
    _FillStyle,
    _StrokeStyle,
    _Opacity,
    _Transform,
    _Translate,
    _BeginPath,
    _ClosePath,
    _MoveTo,
    _LineTo,
    _HLineTo,
    _VLineTo,
    _CubicTo,
    _QuadTo,
    _ArcTo,
    _Line,
    _Curve,
    _Ellipse,
    _Arc,
    _Path,
    _Fill,
    _Stroke,
    _Clip,
    _Apply,
    _Clear,
    _Blit>;

export struct DrawList : Vec<DrawCommand> {
    using Vec<DrawCommand>::Vec;
};

} // namespace Karm::Gfx

