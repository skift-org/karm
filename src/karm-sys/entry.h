#pragma once

#ifdef __KARM_SYS_ENTRY_INCLUDED
#    error "karm-sys/entry.h included twice"
#endif

#define __KARM_SYS_ENTRY_INCLUDED

import Karm.Core;
import Karm.Ref;
import Karm.Sys;

#include <karm-core/macros.h>

[[gnu::used]] Karm::Async::Task<> entryPointAsync(Karm::Sys::Context&, Karm::Async::CancellationToken ct);

#include "__entry.h"