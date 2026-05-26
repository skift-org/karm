module;

#include <karm/macros>

export module Karm.Dl;

export import Karm.Dl.Elf;
import Karm.Sys;

namespace Karm::Dl {

Res<> load(Ref::Url url) {
    auto file = try$(Sys::File::open(url));
    auto mmap = try$(Sys::mmap(file));
}

} // namespace Karm::Dl