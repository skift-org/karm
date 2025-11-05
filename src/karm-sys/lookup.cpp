export module Karm.Sys:lookup;

import :_embed;
import :addr;

namespace Karm::Sys {

export [[clang::coro_wrapper]] Async::Task<Vec<Ip>> lookupAsync(Str host) {
    return _Embed::ipLookupAsync(host);
}

} // namespace Karm::Sys
