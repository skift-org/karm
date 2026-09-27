#include <karm/test>

import Karm.Gfx;

namespace Karm::Gfx::Tests {

test$("karm-test-hsl-rgb-conversion-basic") {
    assertEq$(Gfx::hslToRgb(Gfx::Hsl{0, 0, 0}), Gfx::BLACK);
    assertEq$(Gfx::hslToRgb(Gfx::Hsl{0, 0, 1}), Gfx::WHITE);
    assertEq$(Gfx::hslToRgb(Gfx::Hsl{0, 1, .5}), Gfx::Color::fromRgb(255, 0, 0));
    assertEq$(Gfx::hslToRgb(Gfx::Hsl{120, 1, .5}), Gfx::Color::fromRgb(0, 255, 0));
    assertEq$(Gfx::hslToRgb(Gfx::Hsl{240, 1, .5}), Gfx::Color::fromRgb(0, 0, 255));

    assertEq$(Gfx::rgbToHsl(Gfx::BLACK), (Gfx::Hsl{0, 0.0, 0.0}));
    assertEq$(Gfx::rgbToHsl(Gfx::WHITE), (Gfx::Hsl{0, 0.0, 1.0}));
    assertEq$(Gfx::rgbToHsl(Gfx::Color::fromRgb(255, 0, 0)), (Gfx::Hsl{0, 1.0, .5}));
    assertEq$(Gfx::rgbToHsl(Gfx::Color::fromRgb(0, 255, 0)), (Gfx::Hsl{120, 1.0, .5}));
    assertEq$(Gfx::rgbToHsl(Gfx::Color::fromRgb(0, 0, 255)), (Gfx::Hsl{240, 1.0, .5}));

    return Ok();
}

test$("karm-test-hsl-rgb-conversion-arbitrary") {
    assertEq$(Gfx::hslToRgb(Gfx::rgbToHsl(Gfx::Color::fromRgb(1, 123, 32))), Gfx::Color::fromRgb(1, 123, 32));
    assertEq$(Gfx::hslToRgb(Gfx::rgbToHsl(Gfx::Color::fromRgb(119, 172, 235))), Gfx::Color::fromRgb(119, 172, 235));
    assertEq$(Gfx::hslToRgb(Gfx::rgbToHsl(Gfx::Color::fromRgb(31, 253, 29))), Gfx::Color::fromRgb(31, 253, 29));
    assertEq$(Gfx::hslToRgb(Gfx::rgbToHsl(Gfx::Color::fromRgb(84, 219, 4))), Gfx::Color::fromRgb(84, 219, 4));
    assertEq$(Gfx::hslToRgb(Gfx::rgbToHsl(Gfx::Color::fromRgb(19, 67, 199))), Gfx::Color::fromRgb(19, 67, 199));

    return Ok();
}

} // namespace Karm::Gfx::Tests
