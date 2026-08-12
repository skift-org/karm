export module Karm.Scene3d:camera;

import Karm.Core;
import Karm.Math;
import Karm.App.Base;

namespace Karm::Scene3d {

export struct Camera {
    Math::Vec3f position{0, 0, 0};
    f64 yaw = 0;
    f64 pitch = 0;

    f64 speed = 5.0;
    f64 sensitivity = 0.004;

    bool goForward = false, goBack = false;
    bool goLeft = false, goRight = false;
    bool goUp = false, goDown = false;

    // Camera looks along -Z when yaw and pitch are 0
    Math::Vec3f forward() const {
        return {
            -Math::sin(yaw) * Math::cos(pitch),
            Math::sin(pitch),
            -Math::cos(yaw) * Math::cos(pitch),
        };
    }

    Math::Vec3f right() const {
        return {Math::cos(yaw), 0, -Math::sin(yaw)};
    }

    void key(App::Key k, bool pressed) {
        switch (k.code()) {
        case App::Key::W:
            goForward = pressed;
            break;
        case App::Key::S:
            goBack = pressed;
            break;
        case App::Key::A:
            goLeft = pressed;
            break;
        case App::Key::D:
            goRight = pressed;
            break;
        case App::Key::SPACE:
            goUp = pressed;
            break;
        case App::Key::LSHIFT:
        case App::Key::RSHIFT:
            goDown = pressed;
            break;
        default:
            break;
        }
    }

    void look(f64 dx, f64 dy) {
        yaw -= dx * sensitivity;
        pitch -= dy * sensitivity;

        f64 limit = Math::PI / 2 - 0.001;
        pitch = clamp(pitch, -limit, limit);
    }

    void update(f64 dt) {
        auto f = forward();
        auto r = right();

        Math::Vec3f dir{0, 0, 0};
        if (goForward)
            dir = dir + f;
        if (goBack)
            dir = dir - f;
        if (goRight)
            dir = dir + r;
        if (goLeft)
            dir = dir - r;
        if (goUp)
            dir.y += 1;
        if (goDown)
            dir.y -= 1;

        f64 len = Math::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len < 0.0001)
            return;

        f64 step = speed * dt / len;
        position = position + dir * step;
    }

    Math::Mat4f view() const {
        return Math::Mat4f::rotationX(-pitch) *
               Math::Mat4f::rotationY(-yaw) *
               Math::Mat4f::translation(-position.x, -position.y, -position.z);
    }
};

} // namespace Karm::Scene3d