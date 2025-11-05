export module Karm.Sys:pid;

import Karm.Core;

import :file;

namespace Karm::Sys {

export struct Pid {
    virtual ~Pid() = default;
    virtual Res<> kill() = 0;
    virtual Res<> wait() = 0;
};

} // namespace Karm::Sys