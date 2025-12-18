export module Karm.Gfx:icon;

import Karm.Math;
import :canvas;

namespace Karm::Gfx {

export template <typename T>
concept RawIcon = requires(T t) {
    { t.path } -> Meta::Convertible<char const*>;
    { t.size } -> Meta::Convertible<f64>;
    { t.name } -> Meta::Convertible<char const*>;
};

export struct Icon {
    Str path;
    f64 size;
    Str name;

    explicit Icon(Str path, f64 size = 18, Str name = "")
        : path(path), size(size), name(name) {}

    Icon(RawIcon auto const& icon)
        : path(Str::fromNullterminated(icon.path)), size(icon.size), name(Str::fromNullterminated(icon.name)) {}

    void fill(Gfx::Canvas& g, Math::Vec2f pos, isize size) const {
        g.push();
        g.beginPath();
        g.origin(pos.cast<f64>());
        g.scale(size / (f64)this->size);
        auto p = Math::Path::fromSvg(path);
        g.path(p);
        g.fill();
        g.pop();
    }

    void stroke(Gfx::Canvas& g, Math::Vec2f pos, isize size) const {
        g.push();
        g.beginPath();
        g.origin(pos.cast<f64>());
        g.scale(size / (f64)this->size);
        g.path(Math::Path::fromSvg(path));
        g.stroke();
        g.pop();
    }

    void fill(Gfx::Canvas& g, Math::Vec2f pos = {}) const {
        fill(g, pos, size);
    }

    void stroke(Gfx::Canvas& g, Math::Vec2f pos = {}) const {
        stroke(g, pos, size);
    }
};

} // namespace Karm::Gfx
