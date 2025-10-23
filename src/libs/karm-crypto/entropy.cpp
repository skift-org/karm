export module Karm.Crypto:entropy;

import Karm.Core;
import :_embed;

namespace Karm::Crypto {

export Res<> entropy(MutBytes out) {
    return _Embed::entropy(out);
}

} // namespace Karm::Crypto