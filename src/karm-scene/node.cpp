export module Karm.Scene:node;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;

namespace Karm::Scene {

export struct PaintOptions {
    bool showBackgroundGraphics = true;
};

export struct Node {
    isize zIndex = 0;

    virtual ~Node() = default;

    /// Prepare the scene graph for rendering (z-order, prunning, etc...)
    virtual void prepare() {}

    /// The bounding rectangle of the node
    virtual Math::Rectf bound() const { return {}; }

    virtual void paint(Gfx::Canvas&, Math::Rectf, PaintOptions = {}) {}

    virtual void repr(Io::Emit& e) const {
        e("(node z:{})", zIndex);
    }

    Rc<Gfx::Image> snapshot(Math::Vec2i size, f64 density = 1) {
        auto rect = bound();
        auto image = Gfx::Image::alloc(
            size * density,
            Gfx::RGBA8888
        );

        Gfx::CpuCanvas g;
        g.begin(*image);
        g.scale(density);
        g.clear(Gfx::ALPHA);
        paint(g, Math::Rectf{size.cast<f64>()});
        g.end();

        return image;
    }

    String svg(Math::Vec2i size) {
        Gfx::SvgCanvas g;
        g.begin(size.cast<f64>());
        paint(g, Math::Rectf{size.cast<f64>()});
        return g.finalize();
    }
};

} // namespace Karm::Scene
