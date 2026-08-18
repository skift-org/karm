module;

#include <karm/macros>

export module Karm.Gfx:snapshot;

import Karm.Core;
import Karm.Math;
import :canvas;
import :cpu.canvas;
import :svg.canvas;

namespace Karm::Gfx {

export struct Snapshot {
    using Resource = Union<
        Rc<Image>,
        Rc<Prose>,
        Math::Path,
        Fill,
        Stroke,
        Filter,
        Math::Trans2f>;

    enum struct Op : u16 {
        PUSH,
        POP,

        FILL_STYLE,
        STROKE_STYLE,
        OPACITY,
        ORIGIN,
        TRANSFORM,

        BEGIN_PATH,
        CLOSE_PATH,
        MOVE_TO,
        LINE_TO,
        HLINE_TO,
        VLINE_TO,
        CUBIC_TO,
        QUAD_TO,
        ARC_TO,

        LINE,
        CURVE,
        ELLIPSE,
        ARC,

        PATH,
        FILL,
        FILL_RECT,
        FILL_PROSE,
        STROKE,
        CLIP,
        CLIP_RECTI,
        CLIP_RECTF,
        FILTER,
        BLIT,

        CLEAR,

        PLOT_POINT,
        PLOT_EDGE,
        PLOT_RECT,

        _LEN
    };

    struct _State {
        Vec<u8> _buf = {};
        Vec<Resource> _res = {};

        template <typename T>
        Res<T const&> load(i32 id) const {
            if ((u32)id >= _res.len())
                return Error::invalidInput("invalid resource id");
            if (auto it = _res[id].is<T>())
                return Ok(*it);
            return Error::invalidInput("resource type miss-match");
        }

        i32 add(Resource res) {
            auto id = _res.len();
            _res.pushBack(res);
            return id;
        }

        always_inline void emit(Op op, u16 len) {
            emit((i32)((u32)op << 16 | len));
        }

        always_inline void emit(i32 v) {
            auto buf = unionCast<Array<u8, sizeof(i32le)>, i32le>(v);
            _buf.pushBack(buf);
        }

        always_inline void emit(f32 v) {
            auto buf = unionCast<Array<u8, sizeof(f32)>, f32>(v);
            _buf.pushBack(buf);
        }

        always_inline void emit(Resource res) {
            emit(add(res));
        }
    };

    Math::Vec2i _size;
    Rc<_State> _state;

    static Snapshot from(Rc<Image> img) {
        Recorder recorder{img->bound().size()};
        recorder.blit(img->bound(), img->bound(), img);
        return recorder.finalize();
    }

    static Snapshot from(Math::Vec2i size, Color color) {
        Recorder recorder{size};
        recorder.clear(color);
        return recorder.finalize();
    }

    Snapshot(Math::Vec2i size, Rc<_State> state)
        : _size(size), _state(state) {}

    Math::Vec2i size() const {
        return _size;
    }

    void disasm(Io::Emit& e) const {
        Io::BScan s{_state->_buf};
        while (not s.ended()) {
            auto header = static_cast<u32>(s.nextI32le());
            Op op = static_cast<Op>(header >> 16);
            auto len = header & 0xffff;
            if (op < Op::_LEN)
                e("{}", op);
            else
                e("{:#04x}", static_cast<u16>(op));

            switch (op) {
            case Op::PUSH:
            case Op::POP:
            case Op::BEGIN_PATH:
            case Op::CLOSE_PATH:
            case Op::STROKE:
                break;

            case Op::FILL_STYLE:
                e(" #fill:{}", s.nextI32le());
                break;

            case Op::STROKE_STYLE:
                e(" #stroke:{}", s.nextI32le());
                break;

            case Op::OPACITY:
                e(" opacity:{}", s.nextF32());
                break;

            case Op::ORIGIN: {
                auto x = s.nextF32();
                auto y = s.nextF32();
                e(" x:{} y:{}", x, y);
            } break;

            case Op::TRANSFORM:
                e(" #trans:{}", s.nextI32le());
                break;

            case Op::MOVE_TO:
            case Op::LINE_TO: {
                auto x = s.nextF32();
                auto y = s.nextF32();
                auto f = (u32)s.nextI32le();
                e(" x:{} y:{} options:{:08x}", x, y, f);
            } break;

            case Op::HLINE_TO: {
                auto x = s.nextF32();
                auto f = (u32)s.nextI32le();
                e(" x:{} options:{:08x}", x, f);
            } break;

            case Op::VLINE_TO: {
                auto y = s.nextF32();
                auto f = (u32)s.nextI32le();
                e(" y:{} options:{:08x}", y, f);
            } break;

            case Op::CUBIC_TO: {
                auto cp1x = s.nextF32(), cp1y = s.nextF32();
                auto cp2x = s.nextF32(), cp2y = s.nextF32();
                auto x = s.nextF32(), y = s.nextF32();
                auto f = (u32)s.nextI32le();
                e(" cp1x:{} cp1y:{} cp2x:{} cp2y:{} x:{} y:{} options:{:08x}",
                  cp1x, cp1y, cp2x, cp2y, x, y, f);
            } break;

            case Op::QUAD_TO: {
                auto cpx = s.nextF32(), cpy = s.nextF32();
                auto x = s.nextF32(), y = s.nextF32();
                auto f = (u32)s.nextI32le();
                e(" cpx:{} cpy:{} x:{} y:{} options:{:08x}", cpx, cpy, x, y, f);
            } break;

            case Op::ARC_TO: {
                auto rx = s.nextF32(), ry = s.nextF32();
                auto angle = s.nextF32();
                auto x = s.nextF32(), y = s.nextF32();
                auto f = (u32)s.nextI32le();
                e(" rx:{} ry:{} angle:{} x:{} y:{} options:{:08x}", rx, ry, angle, x, y, f);
            } break;

            case Op::LINE: {
                auto sx = s.nextF32(), sy = s.nextF32();
                auto ex = s.nextF32(), ey = s.nextF32();
                e(" sx:{} sy:{} ex:{} ey:{}", sx, sy, ex, ey);
            } break;

            case Op::CURVE: {
                auto ax = s.nextF32(), ay = s.nextF32();
                auto bx = s.nextF32(), by = s.nextF32();
                auto cx = s.nextF32(), cy = s.nextF32();
                auto dx = s.nextF32(), dy = s.nextF32();
                e(" ax:{} ay:{} bx:{} by:{} cx:{} cy:{} dx:{} dy:{}",
                  ax, ay, bx, by, cx, cy, dx, dy);
            } break;

            case Op::ELLIPSE: {
                auto cx = s.nextF32(), cy = s.nextF32();
                auto rx = s.nextF32(), ry = s.nextF32();
                e(" cx:{} cy:{} rx:{} ry:{}", cx, cy, rx, ry);
            } break;

            case Op::ARC: {
                auto cx = s.nextF32(), cy = s.nextF32();
                auto rx = s.nextF32(), ry = s.nextF32();
                auto start = s.nextF32(), end = s.nextF32();
                e(" cx:{} cy:{} rx:{} ry:{} start:{} end:{}", cx, cy, rx, ry, start, end);
            } break;

            case Op::PATH:
                e(" #path:{}", s.nextI32le());
                break;

            case Op::FILL:
                e(" rule:{}", s.nextI32le());
                break;

            case Op::FILL_RECT: {
                auto x = s.nextF32(), y = s.nextF32();
                auto width = s.nextF32(), height = s.nextF32();
                auto a = s.nextF32(), b = s.nextF32();
                auto c = s.nextF32(), d = s.nextF32();
                auto f = s.nextF32(), g = s.nextF32();
                auto h = s.nextF32(), i = s.nextF32();
                e(" x:{} y:{} width:{} height:{}", x, y, width, height);
                e(" a:{} b:{} c:{} d:{} e:{} f:{} g:{} h:{}", a, b, c, d, f, g, h, i);
            } break;

            case Op::FILL_PROSE:
                e(" #prose:{}", s.nextI32le());
                break;

            case Op::CLIP:
                e(" rule:{}", s.nextI32le());
                break;

            case Op::CLIP_RECTI: {
                auto x = s.nextI32le(), y = s.nextI32le();
                auto width = s.nextI32le(), height = s.nextI32le();
                e(" x:{} y:{} width:{} height:{}", x, y, width, height);
            } break;

            case Op::CLIP_RECTF: {
                auto x = s.nextF32(), y = s.nextF32();
                auto width = s.nextF32(), height = s.nextF32();
                e(" x:{} y:{} width:{} height:{}", x, y, width, height);
            } break;

            case Op::FILTER:
                e(" #filter:{}", s.nextI32le());
                break;

            case Op::BLIT: {
                auto sx = s.nextI32le(), sy = s.nextI32le();
                auto sw = s.nextI32le(), sh = s.nextI32le();
                auto dx = s.nextI32le(), dy = s.nextI32le();
                auto dw = s.nextI32le(), dh = s.nextI32le();
                auto img = s.nextI32le();
                e(" srcX:{} srcY:{} srcWidth:{} srcHeight:{}", sx, sy, sw, sh);
                e(" destX:{} destY:{} destWidth:{} destHeight:{}", dx, dy, dw, dh);
                e(" #image:{}", img);
            } break;

            case Op::CLEAR:
                e(" color:{:08x}", (u32)s.nextI32le());
                break;

            case Op::PLOT_POINT: {
                auto x = s.nextI32le(), y = s.nextI32le();
                auto color = (u32)s.nextI32le();
                e(" x:{} y:{} color:{:08x}", x, y, color);
            } break;

            case Op::PLOT_EDGE: {
                auto sx = s.nextI32le(), sy = s.nextI32le();
                auto ex = s.nextI32le(), ey = s.nextI32le();
                auto color = (u32)s.nextI32le();
                e(" sx:{} sy:{} ex:{} ey:{} color:{:08x}", sx, sy, ex, ey, color);
            } break;

            case Op::PLOT_RECT: {
                auto x = s.nextI32le(), y = s.nextI32le();
                auto width = s.nextI32le(), height = s.nextI32le();
                auto color = (u32)s.nextI32le();
                e(" x:{} y:{} width:{} height:{} color:{:08x}", x, y, width, height, color);
            } break;

            default:
                for (auto _ : Iota(len))
                    e(" {:08x}", s.nextU32le());
                break;
            }
            e("\n");
        }
    }

    Res<Rc<Image>> rasterize(f64 density = 1) const {
        auto image = Image::alloc(
            _size * density,
            RGBA8888
        );
        CpuCanvas g;
        g.begin(*image);
        try$(replay(g));
        g.end();
        return Ok(image);
    }

    Res<String> svg() const {
        SvgCanvas g;
        g.begin(_size.cast<f64>());
        try$(replay(g));
        return Ok(g.finalize());
    }

    [[gnu::flatten]] Res<> replay(Canvas& g) const {
        Io::BScan s{_state->_buf};
        while (not s.ended()) {
            auto header = static_cast<u32>(s.nextI32le());
            auto op = static_cast<Op>(header >> 16);
            auto len = header & 0xffff;

            switch (op) {
            case Op::PUSH:
                g.push();
                break;

            case Op::POP:
                g.pop();
                break;

            case Op::FILL_STYLE:
                g.fillStyle(try$(_state->load<Fill>(s.nextI32le())));
                break;

            case Op::STROKE_STYLE:
                g.strokeStyle(try$(_state->load<Stroke>(s.nextI32le())));
                break;

            case Op::OPACITY:
                g.opacity(s.nextF32());
                break;

            case Op::ORIGIN:
                g.origin({
                    s.nextF32(),
                    s.nextF32(),
                });
                break;

            case Op::TRANSFORM:
                g.transform(try$(_state->load<Math::Trans2f>(s.nextI32le())));
                break;

            case Op::BEGIN_PATH:
                g.beginPath();
                break;

            case Op::CLOSE_PATH:
                g.closePath();
                break;

            case Op::MOVE_TO: {
                auto x = s.nextF32();
                auto y = s.nextF32();
                auto f = Flags<Math::Path::Option>::fromUnderlying(s.nextI32le());
                g.moveTo({x, y}, f);
            } break;

            case Op::LINE_TO: {
                auto x = s.nextF32();
                auto y = s.nextF32();
                auto f = Flags<Math::Path::Option>::fromUnderlying(s.nextI32le());
                g.lineTo({x, y}, f);
            } break;

            case Op::HLINE_TO: {
                auto x = s.nextF32();
                auto f = Flags<Math::Path::Option>::fromUnderlying(s.nextI32le());
                g.hlineTo(x, f);
            } break;

            case Op::VLINE_TO: {
                auto y = s.nextF32();
                auto f = Flags<Math::Path::Option>::fromUnderlying(s.nextI32le());
                g.vlineTo(y, f);
            } break;

            case Op::CUBIC_TO: {
                auto bx = s.nextF32(), by = s.nextF32();
                auto cx = s.nextF32(), cy = s.nextF32();
                auto dx = s.nextF32(), dy = s.nextF32();
                auto f = Flags<Math::Path::Option>::fromUnderlying(s.nextI32le());
                g.cubicTo({bx, by}, {cx, cy}, {dx, dy}, f);
            } break;

            case Op::QUAD_TO: {
                auto bx = s.nextF32(), by = s.nextF32();
                auto cx = s.nextF32(), cy = s.nextF32();
                auto f = Flags<Math::Path::Option>::fromUnderlying(s.nextI32le());
                g.quadTo({bx, by}, {cx, cy}, f);
            } break;

            case Op::ARC_TO: {
                auto rx = s.nextF32(), ry = s.nextF32();
                auto a = s.nextF32();
                auto px = s.nextF32(), py = s.nextF32();
                auto f = Flags<Math::Path::Option>::fromUnderlying(s.nextI32le());
                g.arcTo({rx, ry}, a, {px, py}, f);
            } break;

            case Op::LINE: {
                auto sx = s.nextF32(), sy = s.nextF32();
                auto ex = s.nextF32(), ey = s.nextF32();
                g.line({sx, sy, ex, ey});
            } break;

            case Op::CURVE: {
                auto ax = s.nextF32(), ay = s.nextF32();
                auto bx = s.nextF32(), by = s.nextF32();
                auto cx = s.nextF32(), cy = s.nextF32();
                auto dx = s.nextF32(), dy = s.nextF32();
                g.curve({ax, ay, bx, by, cx, cy, dx, dy});
            } break;

            case Op::ARC: {
                auto cx = s.nextF32(), cy = s.nextF32();
                auto rx = s.nextF32(), ry = s.nextF32();
                auto start = s.nextF32(), end = s.nextF32();
                g.arc({cx, cy, rx, ry, start, end});
            } break;

            case Op::ELLIPSE: {
                auto cx = s.nextF32(), cy = s.nextF32();
                auto rx = s.nextF32(), ry = s.nextF32();
                g.ellipse({cx, cy, rx, ry});
            } break;

            case Op::PATH:
                g.path(try$(_state->load<Math::Path>(s.nextI32le())));
                break;

            case Op::FILL_RECT: {
                Math::Rectf rect{
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                };

                Math::Radiif radii{
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                };

                g.fill(rect, radii);
                break;
            }

            case Op::FILL_PROSE: {
                g.fill(try$(_state->load<Rc<Gfx::Prose>>(s.nextI32le())));
                break;
            }

            case Op::FILL: {
                auto rule = static_cast<FillRule>(s.nextI32le());
                g.fill(rule);
                break;
            }

            case Op::STROKE:
                g.stroke();
                break;

            case Op::CLIP:
                g.clip(static_cast<FillRule>(s.nextI32le()));
                break;

            case Op::CLIP_RECTI: {
                Math::Recti rect{
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                };
                g.clip(rect);
            } break;

            case Op::CLIP_RECTF: {
                Math::Rectf rect{
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                    s.nextF32(),
                };
                g.clip(rect);
            } break;

            case Op::FILTER:
                g.apply(try$(_state->load<Filter>(s.nextI32le())));
                break;

            case Op::BLIT: {
                Math::Recti src{
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                };

                Math::Recti dest{
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                };

                auto img = try$(_state->load<Rc<Image>>(s.nextI32le()));
                g.blit(src, dest, img);
            } break;

            case Op::CLEAR: {
                auto color = Color::fromPacked(s.nextI32le());
                g.clear(color);
            } break;

            case Op::PLOT_POINT: {
                Math::Vec2i point{
                    s.nextI32le(),
                    s.nextI32le(),
                };
                auto color = Color::fromPacked(s.nextI32le());
                g.plot(point, color);
            } break;

            case Op::PLOT_EDGE: {
                Math::Edgei edge{
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                };

                auto color = Color::fromPacked(s.nextI32le());
                g.plot(edge, color);
            } break;

            case Op::PLOT_RECT: {
                Math::Recti rect{
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                    s.nextI32le(),
                };

                auto color = Color::fromPacked(s.nextI32le());
                g.plot(rect, color);
            } break;

            default:
                for (auto _ : Iota(len))
                    s.nextI32le();
                break;
            }
        }

        return Ok();
    }

    void repr(Io::Emit& e) const {
        e("(snapshot {} {})", _size, DataSize{_state->_buf.len()});
    }

    struct Recorder : Canvas {
        Math::Vec2i _size;
        Rc<_State> _state;

        Recorder(Math::Vec2i size)
            : _size(size), _state(makeRc<_State>()) {}

        Snapshot finalize() {
            return {_size, std::move(_state)};
        }

        void push() override {
            _state->emit(Op::PUSH, 0);
        }

        void pop() override {
            _state->emit(Op::POP, 0);
        }

        void fillStyle(Fill style) override {
            _state->emit(Op::FILL_STYLE, 1);
            _state->emit(style);
        }

        void strokeStyle(Stroke style) override {
            _state->emit(Op::STROKE_STYLE, 1);
            _state->emit(style);
        }

        void opacity(f64 opacity) override {
            _state->emit(Op::OPACITY, 1);
            _state->emit(static_cast<f32>(opacity));
        }

        void origin(Math::Vec2f p) override {
            _state->emit(Op::ORIGIN, 2);
            _state->emit(static_cast<f32>(p.x));
            _state->emit(static_cast<f32>(p.y));
        }

        void transform(Math::Trans2f trans) override {
            _state->emit(Op::TRANSFORM, 1);
            _state->emit(trans);
        }

        void beginPath() override {
            _state->emit(Op::BEGIN_PATH, 0);
        }

        void closePath() override {
            _state->emit(Op::CLOSE_PATH, 0);
        }

        void moveTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
            _state->emit(Op::MOVE_TO, 3);
            _state->emit((f32)p.x);
            _state->emit((f32)p.y);
            _state->emit(options.raw());
        }

        void lineTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
            _state->emit(Op::LINE_TO, 3);
            _state->emit((f32)p.x);
            _state->emit((f32)p.y);
            _state->emit(options.raw());
        }

        void hlineTo(f64 x, Flags<Math::Path::Option> options) override {
            _state->emit(Op::HLINE_TO, 2);
            _state->emit((f32)x);
            _state->emit(options.raw());
        }

        void vlineTo(f64 y, Flags<Math::Path::Option> options) override {
            _state->emit(Op::VLINE_TO, 2);
            _state->emit((f32)y);
            _state->emit(options.raw());
        }

        void cubicTo(Math::Vec2f cp1, Math::Vec2f cp2, Math::Vec2f p, Flags<Math::Path::Option> options) override {
            _state->emit(Op::CUBIC_TO, 7);
            _state->emit((f32)cp1.x);
            _state->emit((f32)cp1.y);
            _state->emit((f32)cp2.x);
            _state->emit((f32)cp2.y);
            _state->emit((f32)p.x);
            _state->emit((f32)p.y);
            _state->emit(options.raw());
        }

        void quadTo(Math::Vec2f cp, Math::Vec2f p, Flags<Math::Path::Option> options) override {
            _state->emit(Op::QUAD_TO, 5);
            _state->emit((f32)cp.x);
            _state->emit((f32)cp.y);
            _state->emit((f32)p.x);
            _state->emit((f32)p.y);
            _state->emit(options.raw());
        }

        void arcTo(Math::Vec2f radius, f64 angle, Math::Vec2f p, Flags<Math::Path::Option> options) override {
            _state->emit(Op::ARC_TO, 6);
            _state->emit((f32)radius.x);
            _state->emit((f32)radius.y);
            _state->emit((f32)angle);
            _state->emit((f32)p.x);
            _state->emit((f32)p.y);
            _state->emit(options.raw());
        }

        void line(Math::Edgef edge) override {
            _state->emit(Op::LINE, 4);
            _state->emit((f32)edge.sx);
            _state->emit((f32)edge.sy);
            _state->emit((f32)edge.ex);
            _state->emit((f32)edge.ey);
        }

        void curve(Math::Curvef curve) override {
            _state->emit(Op::CURVE, 8);
            _state->emit((f32)curve.ax);
            _state->emit((f32)curve.ay);
            _state->emit((f32)curve.bx);
            _state->emit((f32)curve.by);
            _state->emit((f32)curve.cx);
            _state->emit((f32)curve.cy);
            _state->emit((f32)curve.dx);
            _state->emit((f32)curve.dy);
        }

        void arc(Math::Arcf arc) override {
            _state->emit(Op::ARC, 6);
            _state->emit((f32)arc.cx);
            _state->emit((f32)arc.cy);
            _state->emit((f32)arc.rx);
            _state->emit((f32)arc.ry);
            _state->emit((f32)arc.s);
            _state->emit((f32)arc.e);
        }

        void ellipse(Math::Ellipsef ellipse) override {
            _state->emit(Op::ELLIPSE, 4);
            _state->emit((f32)ellipse.cx);
            _state->emit((f32)ellipse.cy);
            _state->emit((f32)ellipse.rx);
            _state->emit((f32)ellipse.ry);
        }

        void path(Math::Path const& path) override {
            _state->emit(Op::PATH, 1);
            _state->emit(path);
        }

        void fill(Math::Recti r, Math::Radiif radii) override {
            _state->emit(Op::FILL_RECT, 12);

            _state->emit((f32)r.x);
            _state->emit((f32)r.y);
            _state->emit((f32)r.width);
            _state->emit((f32)r.height);

            _state->emit((f32)radii.a);
            _state->emit((f32)radii.b);
            _state->emit((f32)radii.c);
            _state->emit((f32)radii.d);
            _state->emit((f32)radii.e);
            _state->emit((f32)radii.f);
            _state->emit((f32)radii.g);
            _state->emit((f32)radii.h);
        }

        void fill(Rc<Prose> prose) override {
            _state->emit(Op::FILL_PROSE, 1);
            _state->emit(prose);
        }

        void fill(FillRule rule) override {
            _state->emit(Op::FILL, 1);
            _state->emit(toUnderlyingType(rule));
        }

        void stroke() override {
            _state->emit(Op::STROKE, 0);
        }

        void apply(Filter filter) override {
            _state->emit(Op::FILTER, 1);
            _state->emit(filter);
        }

        void clip(Math::Recti r) override {
            _state->emit(Op::CLIP_RECTI, 4);
            _state->emit((i32)r.x);
            _state->emit((i32)r.y);
            _state->emit((i32)r.width);
            _state->emit((i32)r.height);
        }

        void clip(Math::Rectf r) override {
            _state->emit(Op::CLIP_RECTF, 4);
            _state->emit((f32)r.x);
            _state->emit((f32)r.y);
            _state->emit((f32)r.width);
            _state->emit((f32)r.height);
        }

        void clip(FillRule rule) override {
            _state->emit(Op::CLIP, 1);
            _state->emit(toUnderlyingType(rule));
        }

        void clear(Color color) override {
            _state->emit(Op::CLEAR, 1);
            _state->emit((i32)color.packed());
        }

        void clear(Math::Recti rect, Color color) override {
            push();
            Canvas::clip(rect);
            clear(color);
            pop();
        }

        void plot(Math::Edgei edge, Color color) override {
            _state->emit(Op::PLOT_EDGE, 5);
            _state->emit((i32)edge.sx);
            _state->emit((i32)edge.sy);
            _state->emit((i32)edge.ex);
            _state->emit((i32)edge.ey);
            _state->emit((i32)color.packed());
        }

        void plot(Math::Recti rect, Color color) override {
            _state->emit(Op::PLOT_RECT, 5);
            _state->emit((i32)rect.x);
            _state->emit((i32)rect.y);
            _state->emit((i32)rect.width);
            _state->emit((i32)rect.height);
            _state->emit((i32)color.packed());
        }

        void plot(Math::Vec2i point, Color color) override {
            _state->emit(Op::PLOT_POINT, 3);
            _state->emit((i32)point.x);
            _state->emit((i32)point.y);
            _state->emit((i32)color.packed());
        }

        void blit(Math::Recti src, Math::Recti dest, Rc<Image> surface) override {
            _state->emit(Op::BLIT, 9);

            _state->emit(static_cast<i32>(src.x));
            _state->emit(static_cast<i32>(src.y));
            _state->emit(static_cast<i32>(src.width));
            _state->emit(static_cast<i32>(src.height));

            _state->emit(static_cast<i32>(dest.x));
            _state->emit(static_cast<i32>(dest.y));
            _state->emit(static_cast<i32>(dest.width));
            _state->emit(static_cast<i32>(dest.height));

            _state->emit(surface);
        }
    };
};

} // namespace Karm::Gfx
