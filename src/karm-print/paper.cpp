export module Karm.Print:paper;

import Karm.Core;
import Karm.Math;

namespace Karm::Print {

export constexpr f64 DPI = 96.0;
export constexpr f64 MM_PER_INCH = 25.4;

export constexpr Au mmToAu(f64 mm) {
    return Au{mm * (DPI / MM_PER_INCH)};
}

export enum struct Orientation {
    PORTRAIT,
    LANDSCAPE,

    _LEN,
};

export Orientation orientationFromSize(Vec2Au size) {
    return (size.height >= size.width) ? Orientation::PORTRAIT : Orientation::LANDSCAPE;
}

export struct PaperStock {
    // Size in CSS pixels at 96 DPI
    Str name;
    Au minorAxis;
    Au majorAxis;

    PaperStock static custom(Au minorAxis, Au majorAxis) {
        return PaperStock{
            .name = "Custom",
            .minorAxis = minorAxis,
            .majorAxis = majorAxis,
        };
    }

    Vec2Au size(Orientation orientation) const {
        if (orientation == Orientation::PORTRAIT) {
            return {minorAxis, majorAxis};
        } else {
            return {majorAxis, minorAxis};
        }
    }

    f64 aspect(Orientation orientation) const {
        if (orientation == Orientation::PORTRAIT) {
            return minorAxis / majorAxis;
        } else {
            return majorAxis / minorAxis;
        }
    }

    void repr(Io::Emit& e) const {
        e("(paper {} {}x{})", name, minorAxis, majorAxis);
    }
};

export struct PaperSeries {
    Str name;
    Slice<PaperStock const> stocks;
};

// MARK: ISO Series ------------------------------------------------------------

export constexpr PaperStock A0 = {"A0", mmToAu(841.0), mmToAu(1189.0)};
export constexpr PaperStock A1 = {"A1", mmToAu(594.0), mmToAu(841.0)};
export constexpr PaperStock A2 = {"A2", mmToAu(420.0), mmToAu(594.0)};
export constexpr PaperStock A3 = {"A3", mmToAu(297.0), mmToAu(420.0)};
export constexpr PaperStock A4 = {"A4", mmToAu(210.0), mmToAu(297.0)};
export constexpr PaperStock A5 = {"A5", mmToAu(148.0), mmToAu(210.0)};
export constexpr PaperStock A6 = {"A6", mmToAu(105.0), mmToAu(148.0)};
export constexpr PaperStock A7 = {"A7", mmToAu(74.0), mmToAu(105.0)};
export constexpr PaperStock A8 = {"A8", mmToAu(52.0), mmToAu(74.0)};
export constexpr PaperStock A9 = {"A9", mmToAu(37.0), mmToAu(52.0)};
export constexpr PaperStock A10 = {"A10", mmToAu(26.0), mmToAu(37.0)};

export constexpr Array _A_SERIES = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10};
export constexpr PaperSeries A_SERIES = {"A Series", _A_SERIES};

export constexpr PaperStock B0 = {"B0", mmToAu(1000.0), mmToAu(1414.0)};
export constexpr PaperStock B1 = {"B1", mmToAu(707.0), mmToAu(1000.0)};
export constexpr PaperStock B2 = {"B2", mmToAu(500.0), mmToAu(707.0)};
export constexpr PaperStock B3 = {"B3", mmToAu(353.0), mmToAu(500.0)};
export constexpr PaperStock B4 = {"B4", mmToAu(250.0), mmToAu(353.0)};
export constexpr PaperStock B5 = {"B5", mmToAu(176.0), mmToAu(250.0)};
export constexpr PaperStock B6 = {"B6", mmToAu(125.0), mmToAu(176.0)};
export constexpr PaperStock B7 = {"B7", mmToAu(88.0), mmToAu(125.0)};
export constexpr PaperStock B8 = {"B8", mmToAu(62.0), mmToAu(88.0)};
export constexpr PaperStock B9 = {"B9", mmToAu(33.0), mmToAu(62.0)};
export constexpr PaperStock B10 = {"B10", mmToAu(31.0), mmToAu(44.0)};

export constexpr Array _B_SERIES = {B0, B1, B2, B3, B4, B5, B6, B7, B8, B9, B10};
export constexpr PaperSeries B_SERIES = {"B Series", _B_SERIES};

export constexpr PaperStock C0 = {"C0", mmToAu(917.0), mmToAu(1297.0)};
export constexpr PaperStock C1 = {"C1", mmToAu(648.0), mmToAu(917.0)};
export constexpr PaperStock C2 = {"C2", mmToAu(458.0), mmToAu(648.0)};
export constexpr PaperStock C3 = {"C3", mmToAu(324.0), mmToAu(458.0)};
export constexpr PaperStock C4 = {"C4", mmToAu(229.0), mmToAu(324.0)};
export constexpr PaperStock C5 = {"C5", mmToAu(162.0), mmToAu(229.0)};
export constexpr PaperStock C6 = {"C6", mmToAu(114.0), mmToAu(162.0)};
export constexpr PaperStock C7 = {"C7", mmToAu(81.0), mmToAu(114.0)};
export constexpr PaperStock C8 = {"C8", mmToAu(57.0), mmToAu(81.0)};
export constexpr PaperStock C9 = {"C9", mmToAu(40.0), mmToAu(57.0)};
export constexpr PaperStock C10 = {"C10", mmToAu(28.0), mmToAu(40.0)};

export constexpr Array _C_SERIES = {C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, C10};
export constexpr PaperSeries C_SERIES = {"C Series", _C_SERIES};

// MARK: JIS B-Series: Japanese Variation of ISO B Series  -----------------------------

export constexpr PaperStock JIS_B0 = {"JIS-B0", mmToAu(1030.0), mmToAu(1456.0)};
export constexpr PaperStock JIS_B1 = {"JIS-B1", mmToAu(728.0), mmToAu(1030.0)};
export constexpr PaperStock JIS_B2 = {"JIS-B2", mmToAu(515.0), mmToAu(728.0)};
export constexpr PaperStock JIS_B3 = {"JIS-B3", mmToAu(364.0), mmToAu(515.0)};
export constexpr PaperStock JIS_B4 = {"JIS-B4", mmToAu(257.0), mmToAu(364.0)};
export constexpr PaperStock JIS_B5 = {"JIS-B5", mmToAu(182.0), mmToAu(257.0)};
export constexpr PaperStock JIS_B6 = {"JIS-B6", mmToAu(128.0), mmToAu(182.0)};
export constexpr PaperStock JIS_B7 = {"JIS-B7", mmToAu(91.0), mmToAu(128.0)};
export constexpr PaperStock JIS_B8 = {"JIS-B8", mmToAu(64.0), mmToAu(91.0)};
export constexpr PaperStock JIS_B9 = {"JIS-B9", mmToAu(45.0), mmToAu(64.0)};
export constexpr PaperStock JIS_B10 = {"JIS-B10", mmToAu(32.0), mmToAu(45.0)};
export constexpr PaperStock JIS_B11 = {"JIS-B11", mmToAu(22.0), mmToAu(32.0)};
export constexpr PaperStock JIS_B12 = {"JIS-B12", mmToAu(16.0), mmToAu(22.0)};

export constexpr Array _JIS_B_SERIES = {JIS_B0, JIS_B1, JIS_B2, JIS_B3, JIS_B4, JIS_B5, JIS_B6, JIS_B7, JIS_B8, JIS_B9, JIS_B10, JIS_B11, JIS_B12};
export constexpr PaperSeries JIS_B_SERIES = {"JIS-B Series", _JIS_B_SERIES};

// MARK: US Series -------------------------------------------------------------

export constexpr PaperStock EXECUTIVE = {"Executive", mmToAu(190.5), mmToAu(254.0)};
export constexpr PaperStock FOLIO = {"Folio", mmToAu(210.0), mmToAu(330.0)};
export constexpr PaperStock LEGAL = {"Legal", mmToAu(215.9), mmToAu(355.6)};
export constexpr PaperStock LETTER = {"Letter", mmToAu(215.9), mmToAu(279.4)};
export constexpr PaperStock TABLOID = {"Tabloid", mmToAu(279.4), mmToAu(431.8)};
export constexpr PaperStock LEDGER = {"Ledger", mmToAu(431.8), mmToAu(279.4)};

export constexpr Array _US_SERIES = {EXECUTIVE, FOLIO, LEGAL, LETTER, TABLOID, LEDGER};
export constexpr PaperSeries US_SERIES = {"US Series", _US_SERIES};

// MARK: Envelope Series -------------------------------------------------------

export constexpr PaperStock C5E = {"C5E", mmToAu(229.0), mmToAu(162.0)};
export constexpr PaperStock COMM10E = {"Comm10E", mmToAu(105.0), mmToAu(241.0)};
export constexpr PaperStock DLE = {"DLE", mmToAu(110.0), mmToAu(220.0)};

export constexpr Array _ENVELOPE_SERIES = {C5E, COMM10E, DLE};
export constexpr PaperSeries ENVELOPE_SERIES = {"Envelope Series", _ENVELOPE_SERIES};

// MARK: All Paper Stocks ------------------------------------------------------

export constexpr Array SERIES = {
    A_SERIES,
    B_SERIES,
    C_SERIES,
    JIS_B_SERIES,
    US_SERIES,
    ENVELOPE_SERIES,
};

export Res<PaperStock> findPaperStock(Str name) {
    for (auto const& series : SERIES) {
        for (auto const& stock : series.stocks) {
            if (eqCi(stock.name, name))
                return Ok(stock);
        }
    }
    return Error::invalidData("unknown paper stock");
}

// MARK: Print Settings --------------------------------------------------------

export struct Margins {
    enum struct Named {
        DEFAULT,
        NONE,
        MINIMUM,
        CUSTOM,

        _LEN
    };
    using enum Named;
    Named named;
    InsetsAu custom = mmToAu(20.0);

    Margins(Named named)
        : named(named) {}

    Margins(InsetsAu custom)
        : named(Named::CUSTOM), custom(custom) {}

    bool operator==(Named named) const {
        return this->named == named;
    }

    void repr(Io::Emit& e) const {
        e("{}", named);
    }
};

export struct Settings {
    PaperStock stock = A4;
    Orientation orientation = Orientation::PORTRAIT;
    Margins margins = Margins::DEFAULT;
    f64 scale = 1.;
    bool headerFooter = true;
    bool backgroundGraphics = true;

    Vec2Au pageSize() const {
        return stock.size(orientation);
    }

    f64 pageAspectRatio() const {
        return stock.aspect(orientation);
    }
};

} // namespace Karm::Print
