export module Karm.Scene:box;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;

import :node;

namespace Karm::Scene {

export struct Box : Node {
    Math::Rectf _bound;
    Gfx::Borders _borders;
    Gfx::Outline _outline;
    Vec<Gfx::Fill> _backgrounds;

    Box(Math::Rectf bound,
        Gfx::Borders borders,
        Gfx::Outline outline,
        Vec<Gfx::Fill> backgrounds)
        : _bound(bound), _borders(borders), _outline(outline), _backgrounds(backgrounds) {
    }

    Math::Rectf bound() const override {
        return _bound;
    }

    void paint(Gfx::Canvas& ctx, Math::Rectf r, PaintOptions o) override {
        if (not r.collide(bound()))
            return;

        auto radii = _borders.radii.reduceOverlap(_bound.size());
        if (o.showBackgroundGraphics) {
            for (auto& background : _backgrounds) {
                ctx.fillStyle(background);
                ctx.fill(_bound, radii);
            }
        }

        _borders.paint(ctx, _bound);
        _outline.paint(ctx, _bound, radii);
    }

    void repr(Io::Emit& e) const override {
        e("(box z:{} {} {} {})", zIndex, _bound, _borders.radii, _backgrounds);
    }
};

} // namespace Karm::Scene
