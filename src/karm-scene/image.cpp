export module Karm.Scene:image;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;

import :node;

namespace Karm::Scene {

export struct Image : Node {
    Math::Rectf _bound;
    Rc<Gfx::Image> _image;
    Math::Radiif _radii;

    Image(Math::Rectf bound, Rc<Gfx::Image> _image, Math::Radiif radii = {})
        : _bound(bound), _image(_image), _radii(radii) {
    }

    Math::Rectf bound() const override {
        return _bound;
    }

    void paint(Gfx::Canvas& ctx, Math::Rectf r, PaintOptions) override {
        if (not r.collide(bound()))
            return;

        if (not _radii.zero()) {
            ctx.fillStyle(_image);
            ctx.fill(bound(), _radii);
        } else {
            ctx.blit(_bound.cast<isize>(), _image);
        }
    }

    void repr(Io::Emit& e) const override {
        e("(image z:{})", zIndex);
    }
};

} // namespace Karm::Scene
