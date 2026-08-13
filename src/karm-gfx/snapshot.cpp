module;

#include <karm/macros>

export module Karm.Gfx:snapshot;

import Karm.Core;
import Karm.Math;
import :canvas;

namespace Karm::Gfx {

export struct Snapshot : Canvas {
    using Resource = Union<
        Rc<Image>,
        Math::Path,
        Fill,
        Stroke,
        Filter,
        Math::Trans2f>;

    enum struct Op : u32 {
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

        PATH,
        FILL,
        STROKE,
        CLIP,
        FILTER,
        BLIT,

        CLEAR,

        PLOT_POINT,
        PLOT_EDGE,
        PLOT_RECT,

        _LEN
    };

    struct _State {
        Vec<u8> _buf;
        Vec<Resource> _res;

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

        always_inline void emit(Op op) {
            emit((i32)op);
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

    Snapshot(Math::Vec2i size)
        : _size(size), _state(makeRc<_State>()) {}

    // MARK: Replay ------------------------------------------------------------

    [[gnu::flatten]] Res<> replay(Canvas& g) const {
        Io::BScan s{_state->_buf};
        while (not s.ended()) {
            switch (static_cast<Op>(s.nextI32le())) {
            case Op::PUSH:
                g.push();
                break;

            case Op::POP:
                g.pop();
                break;

            case Op::FILL_STYLE:
                g.fill(try$(_state->load<Fill>(s.nextI32le())));
                break;

            case Op::STROKE_STYLE:
                g.stroke(try$(_state->load<Stroke>(s.nextI32le())));
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

            case Op::PATH:
                g.path(try$(_state->load<Math::Path>(s.nextI32le())));
                break;

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
                return Error::invalidData("invalid opcode");
            }
        }

        return Ok();
    }

    // MARK: Record ------------------------------------------------------------

    void push() override {
        _state->emit(Op::PUSH);
    }

    void pop() override {
        _state->emit(Op::POP);
    }

    void fillStyle(Fill style) override {
        _state->emit(Op::FILL_STYLE);
        _state->emit(style);
    }

    void strokeStyle(Stroke style) override {
        _state->emit(Op::STROKE_STYLE);
        _state->emit(style);
    }

    void opacity(f64 opacity) override {
        _state->emit(Op::OPACITY);
        _state->emit(static_cast<f32>(opacity));
    }

    void origin(Math::Vec2f p) override {
        _state->emit(Op::ORIGIN);
        _state->emit(static_cast<f32>(p.x));
        _state->emit(static_cast<f32>(p.y));
    }

    void transform(Math::Trans2f trans) override {
        _state->emit(Op::TRANSFORM);
        _state->emit(trans);
    }

    void beginPath() override {
        _state->emit(Op::BEGIN_PATH);
    }

    void closePath() override {
        _state->emit(Op::CLOSE_PATH);
    }

    void moveTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _state->emit(Op::MOVE_TO);
        _state->emit((f32)p.x);
        _state->emit((f32)p.y);
        _state->emit(options.raw());
    }

    void lineTo(Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _state->emit(Op::LINE_TO);
        _state->emit((f32)p.x);
        _state->emit((f32)p.y);
        _state->emit(options.raw());
    }

    void hlineTo(f64 x, Flags<Math::Path::Option> options) override {
        _state->emit(Op::HLINE_TO);
        _state->emit((f32)x);
        _state->emit(options.raw());
    }

    void vlineTo(f64 y, Flags<Math::Path::Option> options) override {
        _state->emit(Op::VLINE_TO);
        _state->emit((f32)y);
        _state->emit(options.raw());
    }

    void cubicTo(Math::Vec2f cp1, Math::Vec2f cp2, Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _state->emit(Op::CUBIC_TO);
        _state->emit((f32)cp1.x);
        _state->emit((f32)cp1.y);
        _state->emit((f32)cp2.x);
        _state->emit((f32)cp2.y);
        _state->emit((f32)p.x);
        _state->emit((f32)p.y);
        _state->emit(options.raw());
    }

    void quadTo(Math::Vec2f cp, Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _state->emit(Op::QUAD_TO);
        _state->emit((f32)cp.x);
        _state->emit((f32)cp.y);
        _state->emit((f32)p.x);
        _state->emit((f32)p.y);
        _state->emit(options.raw());
    }

    void arcTo(Math::Vec2f radius, f64 angle, Math::Vec2f p, Flags<Math::Path::Option> options) override {
        _state->emit(Op::ARC_TO);
        _state->emit((f32)radius.x);
        _state->emit((f32)radius.y);
        _state->emit((f32)angle);
        _state->emit((f32)p.x);
        _state->emit((f32)p.y);
        _state->emit(options.raw());
    }

    void path(Math::Path const& path) override {
        _state->emit(Op::PATH);
        _state->emit(path);
    }

    void fill(FillRule rule) override {
        _state->emit(Op::FILL);
        _state->emit(toUnderlyingType(rule));
    }

    void stroke() override {
        _state->emit(Op::STROKE);
    }

    void apply(Filter filter) override {
        _state->emit(Op::FILTER);
        _state->emit(filter);
    }

    void clip(FillRule rule) override {
        _state->emit(Op::CLIP);
        _state->emit(toUnderlyingType(rule));
    }

    void clear(Color color) override {
        _state->emit(Op::CLEAR);
        _state->emit((i32)color.packed());
    }

    void plot(Math::Edgei edge, Color color) override {
        _state->emit(Op::PLOT_EDGE);
        _state->emit((i32)edge.sx);
        _state->emit((i32)edge.sy);
        _state->emit((i32)edge.ex);
        _state->emit((i32)edge.ey);
        _state->emit((i32)color.packed());
    }

    void plot(Math::Recti rect, Color color) override {
        _state->emit(Op::PLOT_RECT);
        _state->emit((i32)rect.x);
        _state->emit((i32)rect.y);
        _state->emit((i32)rect.width);
        _state->emit((i32)rect.height);
        _state->emit((i32)color.packed());
    }

    void plot(Math::Vec2i point, Color color) override {
        _state->emit(Op::PLOT_POINT);
        _state->emit((i32)point.x);
        _state->emit((i32)point.y);
        _state->emit((i32)color.packed());
    }

    void blit(Math::Recti src, Math::Recti dest, Rc<Image> surface) override {
        _state->emit(Op::BLIT);

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

} // namespace Karm::Gfx
