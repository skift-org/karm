export module Karm.Scene:clear;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;

import :proxy;

namespace Karm::Scene {

export struct Clear : Proxy {
    Gfx::Color color;

    Clear(Rc<Node> node, Gfx::Color color)
        : Proxy(node), color(color) {}

    void paint(Gfx::Canvas& g, Math::Rectf r, PaintOptions o) override {
        if (o.showBackgroundGraphics) {
            if (color.alpha == 255) {
                g.clear(color);
            } else {
                g.beginPath();
                g.rect(r);
                g.fill(color);
            }
        }
        _node->paint(g, r, o);
    }

    void repr(Io::Emit& e) const override {
        e("(clear z:{} {} {})", zIndex, color, _node);
    }
};

} // namespace Karm::Scene
