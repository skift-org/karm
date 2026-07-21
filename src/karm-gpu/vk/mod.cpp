import Karm.Gpu;
import Karm.Core;

namespace Karm::Gpu {

struct VkDevice : Device {
    Str backend() override {
        return "vulkan";
    }
};

} // namespace Karm::Gpu
