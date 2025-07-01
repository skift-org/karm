module;

#include <karm-gfx/canvas.h>
#include <karm-pdf/canvas.h>

export module Karm.Print:printer;

import :paper;

namespace Karm::Print {

export struct Printer {
    virtual ~Printer() = default;

    virtual Gfx::Canvas& beginPage(PaperStock paper) = 0;
};

} // namespace Karm::Print
