export module Karm.Kira:chart;

import Karm.Chart;
import Karm.Core;
import Karm.Gfx;
import Karm.Math;
import Karm.Ui;

namespace Karm::Kira {

struct ChartView : Ui::View<ChartView> {
    Chart::Chart _chart;
    Math::Vec2i _size;

    ChartView(Chart::Chart chart, Math::Vec2i size)
        : _chart(std::move(chart)), _size(size) {}

    void reconcile(ChartView& o) override {
        _chart = std::move(o._chart);
        _size = o._size;
    }

    Math::Vec2i size(Math::Vec2i, Ui::Hint) override {
        return _size;
    }

    void paint(Gfx::Canvas& g, Math::Recti) override {
        _chart.paint(g, bound().cast<f64>());
    }
};

export Ui::Child chart(Chart::Chart c, Math::Vec2i size = {480, 280}) {
    return makeRc<ChartView>(std::move(c), size);
}

} // namespace Karm::Kira