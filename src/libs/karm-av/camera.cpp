export module Karm.Av:camera;

import Karm.Core;
import :video;

namespace Karm::Av {

export struct Camera;

namespace _Embed {

Res<Rc<Camera>> openDefaultCamera();

} // namespace _Embed

struct Camera {
    static Res<Rc<Camera>> openDefault() {
        return _Embed::openDefaultCamera();
    }

    virtual ~Camera() = default;

    virtual Res<Rc<VideoStream>> startCapture() = 0;
};

} // namespace Karm::Av