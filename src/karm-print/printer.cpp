export module Karm.Print:printer;

import Karm.Pdf;
import Karm.Gfx;

import :paper;

namespace Karm::Print {

export struct Printer {
    virtual ~Printer() = default;

    virtual Gfx::Canvas& beginPage(Math::Vec2f size) = 0;
};

} // namespace Karm::Print
