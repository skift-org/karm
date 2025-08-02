#pragma once

// This file should not include other headers from karm-sys
import Karm.Core;

namespace Karm::Sys {

using Handle = Distinct<usize, struct _HandleTag>;

static constexpr Handle INVALID = Handle(-1);

} // namespace Karm::Sys
