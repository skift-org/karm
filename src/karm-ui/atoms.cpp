export module Karm.Ui:atoms;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;

namespace Karm::Ui {

static constexpr Math::Au REM(f64 n) {
    return Math::Au{n * 16};
}

// MARK: Spacing ---------------------------------------------------------------

export constexpr Math::Au SPACINGPX = Math::Au{1};
export constexpr Math::Au SPACING0 = Math::Au{0};

export constexpr Math::Au SPACING0_5 = REM(0.125);
export constexpr Math::Au SPACING1 = REM(0.25);
export constexpr Math::Au SPACING1_5 = REM(0.375);
export constexpr Math::Au SPACING2 = REM(0.5);
export constexpr Math::Au SPACING2_5 = REM(0.625);
export constexpr Math::Au SPACING3 = REM(0.75);
export constexpr Math::Au SPACING3_5 = REM(0.875);
export constexpr Math::Au SPACING4 = REM(1);
export constexpr Math::Au SPACING5 = REM(1.25);
export constexpr Math::Au SPACING6 = REM(1.5);
export constexpr Math::Au SPACING7 = REM(1.75);
export constexpr Math::Au SPACING8 = REM(2);
export constexpr Math::Au SPACING9 = REM(2.25);
export constexpr Math::Au SPACING10 = REM(2.5);
export constexpr Math::Au SPACING11 = REM(2.75);
export constexpr Math::Au SPACING12 = REM(3);
export constexpr Math::Au SPACING14 = REM(3.5);
export constexpr Math::Au SPACING16 = REM(4);
export constexpr Math::Au SPACING20 = REM(5);
export constexpr Math::Au SPACING24 = REM(6);
export constexpr Math::Au SPACING28 = REM(7);
export constexpr Math::Au SPACING32 = REM(8);
export constexpr Math::Au SPACING36 = REM(9);
export constexpr Math::Au SPACING40 = REM(10);
export constexpr Math::Au SPACING44 = REM(11);
export constexpr Math::Au SPACING48 = REM(12);
export constexpr Math::Au SPACING52 = REM(13);
export constexpr Math::Au SPACING56 = REM(14);
export constexpr Math::Au SPACING60 = REM(15);
export constexpr Math::Au SPACING64 = REM(16);
export constexpr Math::Au SPACING72 = REM(18);
export constexpr Math::Au SPACING80 = REM(20);
export constexpr Math::Au SPACING96 = REM(24);

// MARK: Colors ----------------------------------------------------------------

export constexpr bool darkMode = true;

export constexpr Gfx::ColorRamp GRAYS = darkMode ? Gfx::ZINC_RAMP : Gfx::ZINC_RAMP.reversed();
export constexpr Gfx::Color GRAY = GRAYS[5];

export constexpr Gfx::Color GRAY50 = GRAYS[0];
export constexpr Gfx::Color GRAY100 = GRAYS[1];
export constexpr Gfx::Color GRAY200 = GRAYS[2];
export constexpr Gfx::Color GRAY300 = GRAYS[3];
export constexpr Gfx::Color GRAY400 = GRAYS[4];
export constexpr Gfx::Color GRAY500 = GRAYS[5];
export constexpr Gfx::Color GRAY600 = GRAYS[6];
export constexpr Gfx::Color GRAY700 = GRAYS[7];
export constexpr Gfx::Color GRAY800 = GRAYS[8];
export constexpr Gfx::Color GRAY900 = GRAYS[9];
export constexpr Gfx::Color GRAY950 = GRAYS[10];

export constexpr Gfx::ColorRamp ACCENTS = darkMode ? Gfx::BLUE_RAMP : Gfx::BLUE_RAMP.reversed();
export constexpr Gfx::Color ACCENT = ACCENTS[5];

export constexpr Gfx::Color ACCENT50 = ACCENTS[0];
export constexpr Gfx::Color ACCENT100 = ACCENTS[1];
export constexpr Gfx::Color ACCENT200 = ACCENTS[2];
export constexpr Gfx::Color ACCENT300 = ACCENTS[3];
export constexpr Gfx::Color ACCENT400 = ACCENTS[4];
export constexpr Gfx::Color ACCENT500 = ACCENTS[5];
export constexpr Gfx::Color ACCENT600 = ACCENTS[6];
export constexpr Gfx::Color ACCENT700 = ACCENTS[7];
export constexpr Gfx::Color ACCENT800 = ACCENTS[8];
export constexpr Gfx::Color ACCENT900 = ACCENTS[9];
export constexpr Gfx::Color ACCENT950 = ACCENTS[10];

} // namespace Karm::Ui
