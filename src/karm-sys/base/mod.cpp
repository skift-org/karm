export module Karm.Sys.Base;

import Karm.Core;
import Karm.Ref;

namespace Karm::Sys {

export enum struct Type {
    DIR,
    FILE,
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

    Ref::Uti uti() const {
        if (type == Type::DIR)
            return Ref::Uti::PUBLIC_DIRECTORY;
        else if (type == Type::FILE)
            return Ref::Uti::fromSuffix(Ref::suffixOf(name));
        else
            return Ref::Uti::PUBLIC_ITEM;
    }

    bool operator==(DirEntry const&) const = default;
};

} // namespace Karm::Sys