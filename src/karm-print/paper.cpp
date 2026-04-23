export module Karm.Print:paper;

import Karm.Core;
import Karm.Math;

namespace Karm::Print {

export Au DPI = 96.0_au;
export Au UNIT = Au{(1 / 25.4)} * DPI;

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
            return f64{Au{minorAxis / majorAxis}};
        } else {
            return f64{Au{majorAxis / minorAxis}};
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

export PaperStock A0 = {"A0", 841.0_au * UNIT, 1189.0_au * UNIT};
export PaperStock A1 = {"A1", 594.0_au * UNIT, 841.0_au * UNIT};
export PaperStock A2 = {"A2", 420.0_au * UNIT, 594.0_au * UNIT};
export PaperStock A3 = {"A3", 297.0_au * UNIT, 420.0_au * UNIT};
export PaperStock A4 = {"A4", 210.0_au * UNIT, 297.0_au * UNIT};
export PaperStock A5 = {"A5", 148.0_au * UNIT, 210.0_au * UNIT};
export PaperStock A6 = {"A6", 105.0_au * UNIT, 148.0_au * UNIT};
export PaperStock A7 = {"A7", 74.0_au * UNIT, 105.0_au * UNIT};
export PaperStock A8 = {"A8", 52.0_au * UNIT, 74.0_au * UNIT};
export PaperStock A9 = {"A9", 37.0_au * UNIT, 52.0_au * UNIT};
export PaperStock A10 = {"A10", 26.0_au * UNIT, 37.0_au * UNIT};

export Array _A_SERIES = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10};
export PaperSeries A_SERIES = {"A Series", _A_SERIES};

export PaperStock B0 = {"B0", 1000.0_au * UNIT, 1414.0_au * UNIT};
export PaperStock B1 = {"B1", 707.0_au * UNIT, 1000.0_au * UNIT};
export PaperStock B2 = {"B2", 500.0_au * UNIT, 707.0_au * UNIT};
export PaperStock B3 = {"B3", 353.0_au * UNIT, 500.0_au * UNIT};
export PaperStock B4 = {"B4", 250.0_au * UNIT, 353.0_au * UNIT};
export PaperStock B5 = {"B5", 176.0_au * UNIT, 250.0_au * UNIT};
export PaperStock B6 = {"B6", 125.0_au * UNIT, 176.0_au * UNIT};
export PaperStock B7 = {"B7", 88.0_au * UNIT, 125.0_au * UNIT};
export PaperStock B8 = {"B8", 62.0_au * UNIT, 88.0_au * UNIT};
export PaperStock B9 = {"B9", 33.0_au * UNIT, 62.0_au * UNIT};
export PaperStock B10 = {"B10", 31.0_au * UNIT, 44.0_au * UNIT};

export Array _B_SERIES = {B0, B1, B2, B3, B4, B5, B6, B7, B8, B9, B10};
export PaperSeries B_SERIES = {"B Series", _B_SERIES};

export PaperStock C0 = {"C0", 917.0_au * UNIT, 1297.0_au * UNIT};
export PaperStock C1 = {"C1", 648.0_au * UNIT, 917.0_au * UNIT};
export PaperStock C2 = {"C2", 458.0_au * UNIT, 648.0_au * UNIT};
export PaperStock C3 = {"C3", 324.0_au * UNIT, 458.0_au * UNIT};
export PaperStock C4 = {"C4", 229.0_au * UNIT, 324.0_au * UNIT};
export PaperStock C5 = {"C5", 162.0_au * UNIT, 229.0_au * UNIT};
export PaperStock C6 = {"C6", 114.0_au * UNIT, 162.0_au * UNIT};
export PaperStock C7 = {"C7", 81.0_au * UNIT, 114.0_au * UNIT};
export PaperStock C8 = {"C8", 57.0_au * UNIT, 81.0_au * UNIT};
export PaperStock C9 = {"C9", 40.0_au * UNIT, 57.0_au * UNIT};
export PaperStock C10 = {"C10", 28.0_au * UNIT, 40.0_au * UNIT};

export Array _C_SERIES = {C0, C1, C2, C3, C4, C5, C6, C7, C8, C9, C10};
export PaperSeries C_SERIES = {"C Series", _C_SERIES};

// MARK: JIS B-Series: Japanese Variation of ISO B Series  -----------------------------

export PaperStock JIS_B0 = {"JIS-B0", 1030.0_au * UNIT, 1456.0_au * UNIT};
export PaperStock JIS_B1 = {"JIS-B1", 728.0_au * UNIT, 1030.0_au * UNIT};
export PaperStock JIS_B2 = {"JIS-B2", 515.0_au * UNIT, 728.0_au * UNIT};
export PaperStock JIS_B3 = {"JIS-B3", 364.0_au * UNIT, 515.0_au * UNIT};
export PaperStock JIS_B4 = {"JIS-B4", 257.0_au * UNIT, 364.0_au * UNIT};
export PaperStock JIS_B5 = {"JIS-B5", 182.0_au * UNIT, 257.0_au * UNIT};
export PaperStock JIS_B6 = {"JIS-B6", 128.0_au * UNIT, 182.0_au * UNIT};
export PaperStock JIS_B7 = {"JIS-B7", 91.0_au * UNIT, 128.0_au * UNIT};
export PaperStock JIS_B8 = {"JIS-B8", 64.0_au * UNIT, 91.0_au * UNIT};
export PaperStock JIS_B9 = {"JIS-B9", 45.0_au * UNIT, 64.0_au * UNIT};
export PaperStock JIS_B10 = {"JIS-B10", 32.0_au * UNIT, 45.0_au * UNIT};
export PaperStock JIS_B11 = {"JIS-B11", 22.0_au * UNIT, 32.0_au * UNIT};
export PaperStock JIS_B12 = {"JIS-B12", 16.0_au * UNIT, 22.0_au * UNIT};

export Array _JIS_B_SERIES = {JIS_B0, JIS_B1, JIS_B2, JIS_B3, JIS_B4, JIS_B5, JIS_B6, JIS_B7, JIS_B8, JIS_B9, JIS_B10, JIS_B11, JIS_B12};
export PaperSeries JIS_B_SERIES = {"JIS-B Series", _JIS_B_SERIES};

// MARK: US Series -------------------------------------------------------------

export PaperStock EXECUTIVE = {"Executive", 190.5_au * UNIT, 254.0_au * UNIT};
export PaperStock FOLIO = {"Folio", 210.0_au * UNIT, 330.0_au * UNIT};
export PaperStock LEGAL = {"Legal", 215.9_au * UNIT, 355.6_au * UNIT};
export PaperStock LETTER = {"Letter", 215.9_au * UNIT, 279.4_au * UNIT};
export PaperStock TABLOID = {"Tabloid", 279.4_au * UNIT, 431.8_au * UNIT};
export PaperStock LEDGER = {"Ledger", 431.8_au * UNIT, 279.4_au * UNIT};

export Array _US_SERIES = {EXECUTIVE, FOLIO, LEGAL, LETTER, TABLOID, LEDGER};
export PaperSeries US_SERIES = {"US Series", _US_SERIES};

// MARK: Envelope Series -------------------------------------------------------

export PaperStock C5E = {"C5E", 229.0_au * UNIT, 162.0_au * UNIT};
export PaperStock COMM10E = {"Comm10E", 105.0_au * UNIT, 241.0_au * UNIT};
export PaperStock DLE = {"DLE", 110.0_au * UNIT, 220.0_au * UNIT};

export Array _ENVELOPE_SERIES = {C5E, COMM10E, DLE};
export PaperSeries ENVELOPE_SERIES = {"Envelope Series", _ENVELOPE_SERIES};

// MARK: All Paper Stocks ------------------------------------------------------

export Array SERIES = {
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
    InsetsAu custom = 20_au * UNIT;

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
