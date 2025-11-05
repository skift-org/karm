module;

#include <karm-core/macros.h>

export module Karm.Sys:dir;

import Karm.Core;
import Karm.Ref;

import :stat;

namespace Karm::Sys {

export struct DirEntry {
    String name;
    Type type;

    bool hidden() const {
        return name[0] == '.';
    }
};

export struct Dir {
    Vec<DirEntry> _entries;
    Ref::Url _url;

    static Res<Dir> open(Ref::Url url) {
        if (url.scheme != "bundle")
            try$(ensureUnrestricted());

        auto entries = try$(_Embed::readDir(url));
        sort(entries, [](auto const& lhs, auto const& rhs) {
            return lhs.name <=> rhs.name;
        });
        return Ok(Dir{entries, url});
    }

    static Res<> create(Ref::Url url) {
        try$(ensureUnrestricted());
        return _Embed::createDir(url);
    }

    static Res<Dir> openOrCreate(Ref::Url url) {
        try$(ensureUnrestricted());
        auto entries = try$(_Embed::readDirOrCreate(url));
        sort(entries, [](auto const& lhs, auto const& rhs) {
            return lhs.name <=> rhs.name;
        });
        return Ok(Dir{entries, url});
    }

    auto const& entries() const { return _entries; }

    auto const& path() const { return _url; }
};

} // namespace Karm::Sys
