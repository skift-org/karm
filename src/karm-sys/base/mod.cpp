export module Karm.Sys.Base;

import Karm.Core;

namespace Karm::Sys {

export enum struct Type {
    FILE,
    DIR,
    OTHER,

    _LEN
};

export struct Stat {
    Type type;
    usize size;
    SystemTime accessTime;
    SystemTime modifyTime;
    SystemTime changeTime;

    bool operator==(Type other) const {
        return type == other;
    }
};

export struct DirEntry {
    String name;
    Type type;

    bool hidden() const {
        return name[0] == '.';
    }
};

} // namespace Karm::Sys