export module Karm.Scene:snapshot;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;

import :node;

namespace Karm::Scene {

export struct Snapshot : Node {
    Gfx::Snapshot _snapshot;

    Snapshot(Gfx::Snapshot snapshot)
        : _snapshot(std::move(snapshot)) {
    }

    Math::Rectf bound() const override {
        return _snapshot.size().cast<f64>();
    }

    void paint(Gfx::Canvas& ctx, Math::Rectf r, PaintOptions) override {
        if (not r.collide(bound()))
            return;
        ctx.push();
        _snapshot.replay(ctx).unwrap();
        ctx.pop();
    }

    void repr(Io::Emit& e) const override {
        e("(snapshot {} z:{})", _snapshot, zIndex);
    }
};

} // namespace Karm::Scene
