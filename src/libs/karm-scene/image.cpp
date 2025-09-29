module;

#include <karm-gfx/canvas.h>

export module Karm.Scene:image;

import Karm.Core;
import :node;

namespace Karm::Scene {

export struct Image : Node {
    Math::Rectf _bound;
    Rc<Gfx::Surface> _surface;
    Math::Radiif _radii;

    Image(Math::Rectf bound, Rc<Gfx::Surface> surface, Math::Radiif radii = {})
        : _bound(bound), _surface(surface), _radii(radii) {
    }

    Math::Rectf bound() override {
        return _bound;
    }

    void paint(Gfx::Canvas& ctx, Math::Rectf r, PaintOptions) override {
        if (not r.colide(bound()))
            return;

        if (not _radii.zero()) {
            ctx.fillStyle(_surface->pixels());
            ctx.fill(bound(), _radii);
        } else {
            ctx.blit(_bound.cast<isize>(), _surface->pixels());
        }
    }

    void repr(Io::Emit& e) const override {
        e("(image z:{})", zIndex);
    }
};

} // namespace Karm::Scene
