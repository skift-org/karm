module;

#include <karm-base/ctype.h>
#include <karm-base/string.h>

export module Karm.Core:glob.fuzzy;

namespace Karm::Glob {

export bool matchFuzzy(Str text, Str pattern) {
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

} // namespace Karm::Glob