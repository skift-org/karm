#pragma once

import Karm.Core;

#include <karm-mime/url.h>

#include "stat.h"

namespace Karm::Sys {

struct DirEntry {
    String name;
    Type type;

    bool hidden() const {
        return name[0] == '.';
    }
};

struct Dir {
    Vec<DirEntry> _entries;
    Mime::Url _url;

    static Res<Dir> open(Mime::Url url);

    static Res<> create(Mime::Url url);

    static Res<Dir> openOrCreate(Mime::Url url);

    auto const& entries() const { return _entries; }

    auto const& path() const { return _url; }
};

} // namespace Karm::Sys
