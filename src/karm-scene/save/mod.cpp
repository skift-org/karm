export module Karm.Scene.Save;

import Karm.Core;
import Karm.Math;
import Karm.Scene;
import Karm.Ref;
import Karm.Image;

namespace Karm::Scene { 

export Res<Vec<u8>> save(Rc<Scene::Node> scene, Math::Vec2i size, ::Karm::Image::Saver const& props = {}) {
    if (props.format == Ref::Uti::PUBLIC_SVG) {
        return Ok(bytes(scene->svg(size)));
    } else {
        auto image = scene->snapshot(size, props.density);
        return ::Karm::Image::save(*image, props);
    }
}

} // namespace Karm::Scene
