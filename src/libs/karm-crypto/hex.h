#pragma once

import Karm.Core;

namespace Karm::Crypto {

Res<> hexEncode(Bytes bytes, Io::TextWriter& out);

Res<String> hexEncode(Bytes bytes);

} // namespace Karm::Crypto
