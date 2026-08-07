export module Karm.Gpu.Rast;

import Karm.Gpu.Base;

namespace Karm::Gpu {

export struct PipelineState {
    Viewport viewport;
    FrontFace frontFace;
    Cull cullMode;
    DepthStencilProps depthStencil = {};
};

} // namespace Karm::Gpu
