module;

#include <karm/macros>

export module Karm.Sys:stat;

import Karm.Core;
import Karm.Ref;
import Karm.Sys.Base;

import :_embed;

namespace Karm::Sys {

export Res<Stat> stat(Ref::Url const& url) {
    return _Embed::stat(url);
}

export Res<bool> isFile(Ref::Url const& url) {
    return Ok(try$(stat(url)).type == Type::FILE);
}

export Res<bool> isDir(Ref::Url const& url) {
    return Ok(try$(stat(url)).type == Type::DIR);
}

export Res<> rename(Ref::Url const& from, Ref::Url const& to) {
    return _Embed::rename(from, to);
}

export Res<> touch(Ref::Url const& url, Opt<SystemTime> const& time = NONE) {
    return _Embed::touch(url, time);
}

export Res<> remove(Ref::Url const& url, Flags<RemoveOption> options) {
    return _Embed::remove(url, options);
}

export Res<> remove(Ref::Url const& url) {
    return remove(url, {RemoveOption::FILE});
}

export Res<> removeDir(Ref::Url const& url) {
    return remove(url, {RemoveOption::DIRECTORY});
}

export Res<> removeAll(Ref::Url const& url) {
    return remove(
        url,
        {
            RemoveOption::FILE,
            RemoveOption::DIRECTORY,
            RemoveOption::RECURSIVE,
        }
    );
}

} // namespace Karm::Sys
