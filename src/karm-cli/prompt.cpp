export module Karm.Cli:prompt;

import Karm.Core;
import Karm.Sys;

using namespace Karm::Literals;

namespace Karm::Cli {

export Res<String> prompt(Str prompt = "]"s) {
    Sys::print("{} ", prompt);
    return Io::readLine<Utf8>(Sys::in());
}

} // namespace Karm::Cli