#pragma once

#include <karm-base/string.h>

namespace Karm::Io {

static inline bool matchFuzzy(Str text, Str pattern) {
    // FIXME: Make this smarter
    usize pi = 0, ti = 0;
    while (pi < pattern.len() &&
           ti < text.len()) {
        if (toAsciiLower(pattern[pi]) == toAsciiLower(text[ti])) {
            pi++;
        }

        ti++;
    }
    return pi == pattern.len();
}

} // namespace Karm::Io
