module;

#include <karm-math/vec.h>

export module Karm.Av:camera;

import Karm.Core;
import :video;

namespace Karm::Av {

export struct Camera;
export struct CameraFormat;

namespace _Embed {

Res<Rc<Camera>> openDefaultCamera();

} // namespace _Embed

export struct CameraInfo {
    String name;
    String driver;
};

export struct CameraFormat {
    Math::Vec2i resolution;
    f64 framerate;
};

struct Camera {
    static Res<Rc<Camera>> openDefault() {
        return _Embed::openDefaultCamera();
    }

    virtual ~Camera() = default;

    virtual Res<Rc<VideoStream>> startCapture() = 0;
    virtual CameraInfo info() const = 0;
    virtual Vec<CameraFormat> formats() const = 0;
};

} // namespace Karm::Av