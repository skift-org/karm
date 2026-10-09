export module Karm.Kira:chart;

import Karm.Chart;
import Karm.Core;
import Karm.Gfx;
import Karm.Math;
import Karm.Ui;

using namespace Karm::Math::Literals;

namespace Karm::Kira {

struct ChartView : Ui::View<ChartView> {
    Chart::Chart _chart;
    Math::Vec2Au _size;

    ChartView(Chart::Chart chart, Math::Vec2Au size)
        : _chart(std::move(chart)), _size(size) {}

    void reconcile(ChartView& o) override {
        _chart = std::move(o._chart);
        _size = o._size;
    }

    Math::Vec2Au size(Math::Vec2Au, Ui::Hint) override {
        return _size;
    }

    void paint(Gfx::Canvas& g, Math::RectAu) override {
        _chart.paint(g, bound().cast<f64>());
    }
};

export Ui::Child chart(Chart::Chart c, Math::Vec2Au size = {480_au, 280_au}) {
    return makeRc<ChartView>(std::move(c), size);
}

} // namespace Karm::Kira