module;

#include <karm-gfx/canvas.h>
#include <karm-io/emit.h>

export module Karm.Scene:image;

import Karm.Image;
import :node;

namespace Karm::Scene {

export struct Image : Node {
    Math::Rectf _bound;
    ::Image::Picture _picture;

    Image(Math::Rectf bound, ::Image::Picture picture)
        : _bound(bound), _picture(std::move(picture)) {
    }

    Math::Rectf bound() override {
        return _bound;
    }

    void paint(Gfx::Canvas& ctx, Math::Rectf r, PaintOptions) override {
        if (not r.colide(bound()))
            return;

        ctx.blit(_bound.cast<isize>(), _picture.pixels());
    }

    void repr(Io::Emit& e) const override {
        e("(image z:{})", zIndex);
    }
};

} // namespace Karm::Scene
