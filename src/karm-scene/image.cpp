export module Karm.Scene:image;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;

import :node;

namespace Karm::Scene {

export struct Image : Node {
    Math::Rectf _bound;
    Rc<Gfx::ImageBlob> _blob;
    Math::Radiif _radii;

    Image(Math::Rectf bound, Rc<Gfx::ImageBlob> blob, Math::Radiif radii = {})
        : _bound(bound), _blob(blob), _radii(radii) {
    }

    Math::Rectf bound() override {
        return _bound;
    }

    void paint(Gfx::Canvas& ctx, Math::Rectf r, PaintOptions) override {
        if (not r.colide(bound()))
            return;

        // if (not _radii.zero()) {
        //     ctx.fillStyle(_surface->pixels());
        //     ctx.fill(bound(), _radii);
        // } else {
        ctx.blit(_bound.cast<isize>(), _blob);
        // }
    }

    void repr(Io::Emit& e) const override {
        e("(image z:{})", zIndex);
    }
};

} // namespace Karm::Scene
