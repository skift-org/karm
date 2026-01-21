module;

#include <karm-core/macros.h>

export module Karm.Sys:stat;

import Karm.Core;
import Karm.Ref;
import Karm.Sys.Base;

import :_embed;
import :sandbox;

namespace Karm::Sys {

export Res<Stat> stat(Ref::Url const& url) {
    if (url.scheme != "bundle")
        try$(ensureUnrestricted());
    return _Embed::stat(url);
}

export Res<bool> isFile(Ref::Url const& url) {
    return Ok(try$(stat(url)).type == Type::FILE);
}

export Res<bool> isDir(Ref::Url const& url) {
    return Ok(try$(stat(url)).type == Type::DIR);
}

} // namespace Karm::Sys
