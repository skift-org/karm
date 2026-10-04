module;

#include <karm/macros>

export module Karm.Sys:info;

import Karm.Core;
import Karm.Ref;

import :proc;
import :_embed;

namespace Karm::Sys {

export struct SysInfo {
    String sysName;
    String sysVersion;

    String kernelName;
    String kernelVersion;

    String hostname;
};

export Res<SysInfo> sysinfo() {
    SysInfo infos;
    try$(_Embed::populate(infos));
    return Ok(infos);
}

export struct MemInfo {
    usize physicalTotal;
    usize physicalFree;

    usize physicalUsed() const {
        return physicalTotal - physicalFree;
    }

    usize swapTotal;
    usize swapFree;

    usize swapUsed() const {
        return swapTotal - swapFree;
    }

};

export Res<MemInfo> meminfo() {
    MemInfo infos;
    try$(_Embed::populate(infos));
    return Ok(infos);
}

export struct Cpu {
    String name;
    String brand;
    String vendor;

    Ticks userTime;
    Ticks systemTime;
    Ticks idleTime;

    usize freq;

    static Res<Vec<Cpu>> list() {
        Vec<Cpu> infos;
        try$(_Embed::populate(infos));
        return Ok(std::move(infos));
    }
};

export struct UserInfo {
    String name;
    Ref::Url home;
    Ref::Url shell;
};

export Res<UserInfo> userinfo() {
    UserInfo infos;
    try$(_Embed::populate(infos));
    return Ok(infos);
}

export Res<Vec<UserInfo>> usersinfo() {
    Vec<UserInfo> infos;
    try$(_Embed::populate(infos));
    return Ok(infos);
}

} // namespace Karm::Sys
