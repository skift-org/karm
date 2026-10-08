export module Karm.Chart;

import Karm.Core;
import Karm.Gfx;
import Karm.Math;
import Karm.Logger;

namespace Karm::Chart {

export struct Serie {
    Vec<f64> values;
    Opt<Gfx::Color> color = {};

    frange valuesRange() const {
        return frange::fromStartEnd(
            (iter(values) | Min()).unwrapOr(0.),
            (iter(values) | Max()).unwrapOr(0.)
        );
    }

    usize len() const {
        return values.len();
    }
};

export struct Data {
    Vec<Serie> series;

    frange valuesRange() const {
        frange r = {};
        for (auto& [s, i] : iter(series) | Index())
            r = i == 0 ? s.valuesRange() : r.merge(s.valuesRange());
        return r;
    }

    usize len() const {
        return (
                   iter(series) | Select([](auto& s) {
                       return s.len();
                   }) |
                   Max()
        )
            .unwrapOr(0);
    }
};

export struct Style {
    Opt<f64> min = NONE;
    Opt<f64> max = NONE;
};

export struct Domain {
    Math::Rectf bound;
    frange valuesRange;
    usize len;

    Math::Vec2f map(f64 v, usize i) {
        return bound.topStart() +
               Math::Vec2f{
                   bound.width * (i / static_cast<f64>(len - 1)),
                   bound.height * (1 - ((v - valuesRange.start) / valuesRange.size)),
               };
    }
};

struct Theme {
    Vec<Gfx::Color> palette = {
        Gfx::BLUE500,
        Gfx::EMERALD500,
        Gfx::AMBER500,
        Gfx::ROSE500,
        Gfx::VIOLET500,
    };
};

export struct Chart {
    Data data = {};
    Theme theme = {};
    Style style = {};

    void paint(Gfx::Canvas& g, Math::Rectf bound) {
        if (not data.series)
            return;

        auto valuesRange = data.valuesRange();

        Domain domain{
            bound,
            frange::fromStartEnd(
                style.min.unwrapOr(valuesRange.start),
                style.max.unwrapOr(valuesRange.end())
            ),
            data.len()
        };

        for (auto& [serie, i] : iter(data.series) | Index()) {
            auto color = serie.color.unwrapOr(theme.palette.index(i).unwrapOr(Gfx::BLUE));

            // fill
            g.beginPath();
            g.moveTo(domain.map(first(serie.values), 0));
            for (auto i : Iota(domain.len)) {
                g.lineTo(domain.map(serie.values[i], i));
            }
            g.lineTo(bound.bottomEnd());
            g.lineTo(bound.bottomStart());
            g.closePath();
            g.fill(color.withOpacity(0.5));

            // stroke
            g.beginPath();
            g.moveTo(domain.map(first(serie.values), 0));
            for (auto i : Iota(domain.len))
                g.lineTo(domain.map(serie.values[i], i));
            g.stroke(Gfx::stroke(color).withWidth(1));
        }
    }
};

} // namespace Karm::Chart