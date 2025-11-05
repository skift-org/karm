module;

#include <karm-core/macros.h>

export module Karm.Av:loader;

import Karm.Core;
import Karm.Ref;
import Karm.Sys;
import :audio;
import :wav;

namespace Karm::Av {

export Res<Rc<Audio>> load(Bytes bytes) {
    if (Wav::sniff(bytes)) {
        Wav::Decoder dec;
        return dec.decode(bytes);
    } else {
        return Error::invalidData("unknown audio format");
    }
}

export Res<Rc<Audio>> load(Ref::Url url) {
    if (url.scheme == "data") {
        auto blob = try$(url.blob);
        return load(blob->data);
    }
    auto file = try$(Sys::File::open(url));
    auto map = try$(Sys::mmap(file));
    return load(map.bytes());
}

} // namespace Karm::Av
