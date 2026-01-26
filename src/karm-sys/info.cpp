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
    try$(ensureUnrestricted());
    SysInfo infos;
    try$(_Embed::populate(infos));
    return Ok(infos);
}

export struct MemInfo {
    usize physicalTotal;
    usize physicalUsed;

    usize virtualTotal;
    usize virtualUsed;

    usize swapTotal;
    usize swapUsed;
};

export Res<MemInfo> meminfo() {
    MemInfo infos;
    try$(_Embed::populate(infos));
    return Ok(infos);
}

export struct CpuInfo {
    String name;
    String brand;
    String vendor;

    usize usage;
    usize freq;
};

export Res<Vec<CpuInfo>> cpusinfo() {
    Vec<CpuInfo> infos;
    try$(_Embed::populate(infos));
    return Ok(infos);
}

export struct UserInfo {
    String name;
    Ref::Url home;
    Ref::Url shell;
};

export Res<UserInfo> userinfo() {
    try$(ensureUnrestricted());
    UserInfo infos;
    try$(_Embed::populate(infos));
    return Ok(infos);
}

export Res<Vec<UserInfo>> usersinfo() {
    try$(ensureUnrestricted());
    Vec<UserInfo> infos;
    try$(_Embed::populate(infos));
    return Ok(infos);
}

} // namespace Karm::Sys
