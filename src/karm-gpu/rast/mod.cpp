export module Karm.Gpu.Rast;

import Karm.Core;
import Karm.Math;
import Karm.Gpu.Base;

namespace Karm::Gpu {

export struct PipelineState {
    DepthStencilProps depthStencil = {};
    Viewport viewport;
    Opt<Math::Recti> scissor;
    FrontFace frontFace;
    Cull cullMode;
};

} // namespace Karm::Gpu
