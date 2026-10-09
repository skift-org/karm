export module Karm.Ui:layout;

import Karm.App;
import Karm.Gfx;
import Karm.Math;

import :node;
import :view;
import :atoms;

using namespace Karm::Math::Literals;

namespace Karm::Ui {

export constexpr Au UNCONSTRAINED = -1_au;

// MARK: Empty -----------------------------------------------------------------

struct Empty : View<Empty> {
    Math::Vec2Au _size;

    Empty(Math::Vec2Au size)
        : _size(size) {}

    void reconcile(Empty& o) override {
        _size = o._size;
    }

    Math::Vec2Au size(Math::Vec2Au, Hint) override {
        return _size;
    }

    void paint(Gfx::Canvas&, Math::RectAu) override {}
};

export Child empty(Math::Vec2Au size = {}) {
    return makeRc<Empty>(size);
}

export auto cond(bool c) {
    return [c](Child child) {
        if (c)
            return child;
        return empty();
    };
}

// MARK: Bound -----------------------------------------------------------------

struct Bound : ProxyNode<Bound> {
    Math::RectAu _bound;

    Bound(Child child)
        : ProxyNode(child) {}

    Math::RectAu bound() override {
        return _bound;
    }

    void layout(Math::RectAu bound) override {
        _bound = bound;
        child().layout(bound);
    }

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
        return child().size(s, hint);
    }
};

export auto bound() {
    return [](Child child) {
        return makeRc<Bound>(child);
    };
}

struct Placed : ProxyNode<Placed> {
    Math::RectAu _bound;
    Math::RectAu _place;

    Placed(Math::RectAu place, Child child)
        : ProxyNode(child), _place(place) {}

    void reconcile(Placed& o) override {
        _place = o._place;
        ProxyNode::reconcile(o);
    }

    Math::RectAu bound() override {
        return _bound;
    }

    void layout(Math::RectAu bound) override {
        _bound = bound;
        auto place = _place;
        place.xy = place.xy + _bound.xy;
        child().layout(place);
    }

    Math::Vec2Au size(Math::Vec2Au s, Hint) override {
        return s;
    }
};

export auto placed(Math::RectAu bound) {
    return [bound](Child child) {
        return makeRc<Placed>(bound, child);
    };
}

// MARK: Grow ------------------------------------------------------------------

struct Grow : ProxyNode<Grow> {
    isize _grow;

    Grow(Child child)
        : ProxyNode(child), _grow(1) {}

    Grow(isize grow, Child child)
        : ProxyNode(child), _grow(grow) {}

    isize grow() const {
        return _grow;
    }
};

export Child grow(None) {
    return makeRc<Grow>(empty());
}

export auto grow() {
    return [](Child child) {
        return makeRc<Grow>(child);
    };
}

export Child grow(isize grow, None) {
    return makeRc<Grow>(
        grow,
        empty()
    );
}

export auto grow(isize g) {
    return [g](Child child) {
        return makeRc<Grow>(g, child);
    };
}

// MARK: Align -----------------------------------------------------------------

struct Align : ProxyNode<Align> {
    Math::Align _align;

    Align(Math::Align align, Child child) : ProxyNode(child), _align(align) {}

    void layout(Math::RectAu bound) override {
        auto childSize = child().size(
            bound.size(), _child.is<Grow>()
                              ? Hint::MAX
                              : Hint::MIN
        );

        child()
            .layout(_align.apply<Au>(
                Math::Flow::LEFT_TO_RIGHT,
                childSize,
                bound
            ));
    };

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
        if (hint == Hint::MAX)
            return _align.maxSize(child().size(s, hint), s);
        return _align.minSize(child().size(s, hint));
    }
};

export auto align(Math::Align a) {
    return [a](Child child) {
        return makeRc<Align>(a, child);
    };
}

export auto center() {
    return align(Math::Align::CENTER);
}

export auto start() {
    return align(Math::Align::START | Math::Align::VFILL);
}

export auto end() {
    return align(Math::Align::END | Math::Align::VFILL);
}

export auto fit() {
    return align(Math::Align::FIT);
}

export auto cover() {
    return align(Math::Align::COVER);
}

export auto hcenter() {
    return align(Math::Align::HCENTER | Math::Align::TOP);
}

export auto vcenter() {
    return align(Math::Align::VCENTER | Math::Align::START);
}

export auto hcenterFill() {
    return align(Math::Align::HCENTER | Math::Align::VFILL);
}

export auto vcenterFill() {
    return align(Math::Align::VCENTER | Math::Align::HFILL);
}

// MARK: Sizing ----------------------------------------------------------------

struct Sizing : ProxyNode<Sizing> {
    Math::Vec2Au _min;
    Math::Vec2Au _max;
    Math::RectAu _rect;

    Sizing(Math::Vec2Au min, Math::Vec2Au max, Child child)
        : ProxyNode(child), _min(min), _max(max) {}

    Math::RectAu bound() override {
        return _rect;
    }

    void reconcile(Sizing& o) override {
        _min = o._min;
        _max = o._max;
        ProxyNode<Sizing>::reconcile(o);
    }

    void layout(Math::RectAu bound) override {
        _rect = bound;
        child().layout(bound);
    }

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
        if (_max.x != UNCONSTRAINED) {
            s.x = min(s.x, _max.x);
        }

        if (_max.y != UNCONSTRAINED) {
            s.y = min(s.y, _max.y);
        }

        auto result = child().size(s, hint);

        if (_min.x != UNCONSTRAINED) {
            result.x = max(result.x, _min.x);
        }

        if (_max.x != UNCONSTRAINED) {
            result.x = min(result.x, _max.x);
        }

        if (_min.y != UNCONSTRAINED) {
            result.y = max(result.y, _min.y);
        }

        if (_max.y != UNCONSTRAINED) {
            result.y = min(result.y, _max.y);
        }

        return result;
    }
};

export auto sizing(Math::Vec2Au min, Math::Vec2Au max) {
    return [min, max](Child child) {
        return makeRc<Sizing>(min, max, child);
    };
}

export auto minSize(Math::Vec2Au size) {
    return sizing(size, UNCONSTRAINED);
}

export auto maxSize(Math::Vec2Au size) {
    return sizing(UNCONSTRAINED, size);
}

export auto pinSize(Math::Vec2Au size) {
    return sizing(size, size);
}

// MARK: Insets ---------------------------------------------------------------

struct Insets : ProxyNode<Insets> {
    Math::InsetsAu _insets;

    Insets(Math::InsetsAu insets, Child child)
        : ProxyNode(child), _insets(insets) {}

    void reconcile(Insets& o) override {
        _insets = o._insets;
        ProxyNode<Insets>::reconcile(o);
    }

    void paint(Gfx::Canvas& g, Math::RectAu r) override {
        child().paint(g, r);
    }

    void layout(Math::RectAu rect) override {
        child().layout(rect.shrink(_insets));
    }

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
        return child().size(s - _insets.all(), hint) + _insets.all();
    }

    Math::RectAu bound() override {
        return child().bound().grow(_insets);
    }
};

export auto insets(Math::InsetsAu s) {
    return [s](Child child) {
        return makeRc<Insets>(s, child);
    };
}

// MARK: Aspect Ratio ----------------------------------------------------------

struct AspectRatio : ProxyNode<AspectRatio> {
    f64 _ratio;

    AspectRatio(f64 ratio, Child child)
        : ProxyNode(child), _ratio(ratio) {}

    void reconcile(AspectRatio& o) override {
        _ratio = o._ratio;
        ProxyNode::reconcile(o);
    }

    void paint(Gfx::Canvas& g, Math::RectAu r) override {
        child().paint(g, r);
    }

    Math::Vec2Au size(Math::Vec2Au s, Hint) override {
        if (s.x < s.y)
            return {s.x, s.x * _ratio};

        return {s.y * _ratio, s.y};
    }

    Math::RectAu bound() override {
        return child().bound();
    }
};

export auto aspectRatio(f64 ratio) {
    return [ratio](Child child) {
        return makeRc<AspectRatio>(ratio, child);
    };
}

// MARK: Stack -----------------------------------------------------------------

struct StackLayout : GroupNode<StackLayout> {
    using GroupNode::GroupNode;

    void event(App::Event& e) override {
        if (e.accepted())
            return;

        for (auto& child : mutIterRev(children())) {
            child->event(e);
            if (e.accepted())
                return;
        }
    }

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
        Au w{};
        Au h{};

        for (auto& child : children()) {
            auto childSize = child->size(s, hint);
            w = max(w, childSize.x);
            h = max(h, childSize.y);
        }

        return {w, h};
    }
};

export Child stack(Children children) {
    return makeRc<StackLayout>(children);
}

export Child stack(auto... children) {
    return stack(Children{children...});
}

// MARK: Flow ------------------------------------------------------------------

struct FlowStyle {
    Math::Flow flow = Math::Flow::LEFT_TO_RIGHT;
    Math::Align align = Math::Align::FILL;
    Au gaps{};

    static FlowStyle horizontal(Au gaps = 0_au, Math::Align align = Math::Align::FILL) {
        return FlowStyle{Math::Flow::LEFT_TO_RIGHT, align, gaps};
    }

    static FlowStyle vertical(Au gaps = 0_au, Math::Align align = Math::Align::FILL) {
        return FlowStyle{Math::Flow::TOP_TO_BOTTOM, align, gaps};
    }
};

struct FlowLayout : GroupNode<FlowLayout> {
    using GroupNode::GroupNode;

    FlowStyle _style;

    FlowLayout(FlowStyle style, Children children)
        : GroupNode(children), _style(style) {}

    void reconcile(FlowLayout& o) override {
        _style = o._style;
        GroupNode::reconcile(o);
    }

    Au _computeGrowUnit(Math::RectAu r) {
        Au total = 0_au;
        isize grows = 0;

        for (auto& child : children()) {
            if (child.is<Grow>()) {
                grows += child.expect<Grow>().grow();
            } else {
                total += _style.flow.getX(child->size(r.size(), Hint::MIN));
            }
        }

        Au all = _style.flow.getWidth(r) - _style.gaps * (max(1uz, children().len()) - 1);
        Au growTotal = max(0_au, all - total);
        return growTotal / max(1, grows);
    }

    void layout(Math::RectAu r) override {
        _bound = r;

        Au growUnit = _computeGrowUnit(r);
        Au start = _style.flow.getStart(r);

        for (auto& child : children()) {
            Math::RectAu inner = {};
            auto childSize = child->size(r.size(), Hint::MIN);

            inner = _style.flow.setStart(inner, start);
            if (child.is<Grow>()) {
                inner = _style.flow.setWidth(inner, growUnit * child.expect<Grow>().grow());
            } else {
                inner = _style.flow.setWidth(inner, _style.flow.getX(childSize));
            }

            inner = _style.flow.setTop(inner, _style.flow.getTop(r));
            inner = _style.flow.setBottom(inner, _style.flow.getBottom(r));

            child->layout(_style.align.apply(_style.flow, Math::RectAu{childSize}, inner));
            start += _style.flow.getWidth(inner) + _style.gaps;
        }
    }

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
        Au w{};
        Au h{hint == Hint::MAX ? _style.flow.getY(s) : 0_au};
        bool grow = false;

        for (auto& child : children()) {
            if (child.is<Grow>())
                grow = true;

            auto childSize = child->size(s, Hint::MIN);
            w += _style.flow.getX(childSize);
            h = max(h, _style.flow.getY(childSize));
        }

        w += _style.gaps * (max(1uz, children().len()) - 1);
        if (grow and hint == Hint::MAX) {
            w = max(_style.flow.getX(s), w);
        }

        return _style.flow.orien() == Math::Orien::HORIZONTAL
                   ? Math::Vec2Au{w, h}
                   : Math::Vec2Au{h, w};
    }
};

export Child flow(FlowStyle style, Children children) {
    return makeRc<FlowLayout>(style, children);
}

export Child hflow(Meta::Convertible<Child> auto... children) {
    return flow({.flow = Math::Flow::LEFT_TO_RIGHT}, {children...});
}

export Child hflow(Au gaps, Meta::Convertible<Child> auto... children) {
    return flow({.flow = Math::Flow::LEFT_TO_RIGHT, .gaps = gaps}, {children...});
}

export Child hflow(Au gaps, Math::Align align, Meta::Convertible<Child> auto... children) {
    return flow({.flow = Math::Flow::LEFT_TO_RIGHT, .align = align, .gaps = gaps}, {children...});
}

export Child hflow(Children children) {
    return flow({.flow = Math::Flow::LEFT_TO_RIGHT}, children);
}

export Child hflow(Au gaps, Children children) {
    return flow({.flow = Math::Flow::LEFT_TO_RIGHT, .gaps = gaps}, children);
}

export Child hflow(Au gaps, Math::Align align, Children children) {
    return flow({.flow = Math::Flow::LEFT_TO_RIGHT, .align = align, .gaps = gaps}, children);
}

export Child vflow(Meta::Convertible<Child> auto... children) {
    return flow({.flow = Math::Flow::TOP_TO_BOTTOM}, {children...});
}

export Child vflow(Au gaps, Meta::Convertible<Child> auto... children) {
    return flow({.flow = Math::Flow::TOP_TO_BOTTOM, .gaps = gaps}, {children...});
}

export Child vflow(Au gaps, Math::Align align, Meta::Convertible<Child> auto... children) {
    return flow({.flow = Math::Flow::TOP_TO_BOTTOM, .align = align, .gaps = gaps}, {children...});
}

export Child vflow(Children children) {
    return flow({.flow = Math::Flow::TOP_TO_BOTTOM}, children);
}

export Child vflow(Au gaps, Children children) {
    return flow({.flow = Math::Flow::TOP_TO_BOTTOM, .gaps = gaps}, children);
}

export Child vflow(Au gaps, Math::Align align, Children children) {
    return flow({.flow = Math::Flow::TOP_TO_BOTTOM, .align = align, .gaps = gaps}, children);
}

// MARK: Grid ------------------------------------------------------------------

export struct GridUnit {
    enum _Unit {
        AUTO,
        FIXED,
        GROW,
    };

    _Unit unit;
    Au size = 0_au;
    isize factor = 0;

    static GridUnit auto_() {
        return {AUTO};
    }

    static GridUnit fixed(Au size) {
        return {FIXED, size};
    }

    static GridUnit grow(isize factor = 1) {
        return {GROW, 0_au, factor};
    }

    GridUnit(_Unit unit, Au size = 0_au, isize factor = 0)
        : unit(unit), size(size), factor(factor) {}

    Vec<GridUnit> repeated(usize count) {
        Vec<GridUnit> units{};
        while (count--)
            units.pushBack(*this);
        return units;
    }
};

export struct GridStyle {
    Vec<GridUnit> rows;
    Vec<GridUnit> columns;

    Math::Vec2Au gaps;
    Math::Flow flow = Math::Flow::LEFT_TO_RIGHT;
    Math::Align align = Math::Align::FILL;

    static GridStyle simpleGrow(isize rows, isize columns, Math::Vec2Au gaps = 0_au) {
        return GridStyle{
            GridUnit::grow().repeated(rows),
            GridUnit::grow().repeated(columns),
            gaps,
            Math::Flow::LEFT_TO_RIGHT,
            Math::Align::FILL,
        };
    }

    static GridStyle simpleFixed(Pair<isize, Au> rows, Pair<isize, Au> columns, Math::Vec2Au gaps = {}) {
        return GridStyle{
            GridUnit::fixed(rows.v1).repeated(rows.v0),
            GridUnit::fixed(columns.v1).repeated(columns.v0),
            gaps,
            Math::Flow::LEFT_TO_RIGHT,
            Math::Align::FILL,
        };
    }

    static GridStyle simpleAuto(isize rows, isize columns, Math::Vec2Au gaps = 0_au) {
        return GridStyle{
            GridUnit::auto_().repeated(rows),
            GridUnit::auto_().repeated(columns),
            gaps,
            Math::Flow::LEFT_TO_RIGHT,
            Math::Align::FILL,
        };
    }
};

struct Cell : ProxyNode<Cell> {
    Math::Vec2i _start{};
    Math::Vec2i _end{};

    Cell(Math::Vec2i start, Math::Vec2i end, Child child)
        : ProxyNode(child), _start(start), _end(end) {}

    Math::Vec2i start() const {
        return _start;
    }

    Math::Vec2i end() const {
        return _end;
    }
};

export auto cell(Math::Vec2i pos) {
    return [pos](Child child) {
        return makeRc<Cell>(pos, pos, child);
    };
}

export auto cell(Math::Vec2i start, Math::Vec2i end) {
    return [start, end](Child child) {
        return makeRc<Cell>(start, end, child);
    };
}

struct GridLayout : GroupNode<GridLayout> {
    struct _Dim {
        Au start;
        Au size;

        Au end() const {
            return start + size;
        }
    };

    GridStyle _style;
    Vec<_Dim> _rows;
    Vec<_Dim> _columns;

    GridLayout(GridStyle style, Children children)
        : GroupNode(children), _style(style) {}

    void reconcile(GridLayout& o) override {
        _style = std::move(o._style);
        _rows.clear();
        _columns.clear();
        GroupNode::reconcile(o);
    }

    Au computeGapsRows() {
        return _style.gaps.y * (max(1uz, _style.rows.len()) - 1);
    }

    Au computeGapsColumns() {
        return _style.gaps.x * (max(1uz, _style.columns.len()) - 1);
    }

    Au computeGrowUnitRows(Math::RectAu r) {
        Au total = 0_au;
        isize grows = 0;

        for (auto& row : _style.rows) {
            if (row.unit == GridUnit::GROW) {
                grows += row.factor;
            } else {
                total += row.size;
            }
        }

        Au all = _style.flow.getHeight(r) - computeGapsRows();
        Au growTotal = max(0_au, all - total);

        return growTotal / max(1, grows);
    }

    Au computeGrowUnitColumns(Math::RectAu r) {
        Au total = 0_au;
        isize grows = 0;

        for (auto& column : _style.columns) {
            if (column.unit == GridUnit::GROW) {
                grows += column.factor;
            } else {
                total += column.size;
            }
        }

        Au all = _style.flow.getWidth(r) - computeGapsColumns();
        Au growTotal = max(0_au, all - total);

        return growTotal / max(1, grows);
    }

    void place(Child child, Math::Vec2i pos) {
        place(child, pos, pos);
    }

    void place(Child child, Math::Vec2i start, Math::Vec2i end) {
        auto startRow = _rows[start.y];
        auto startColumn = _columns[start.x];

        auto endRow = _rows[end.y];
        auto endColumn = _columns[end.x];

        auto childRect = Math::RectAu{
            startColumn.start,
            startRow.start,
            endColumn.end() - startColumn.start,
            endRow.end() - startRow.start,
        };

        child->layout(childRect);
    }

    void layout(Math::RectAu r) override {
        _bound = r;

        // compute the dimensions of the grid
        _rows.clear();
        Au growUnitRows = computeGrowUnitRows(r);
        Au row = _style.flow.getTop(r);
        for (auto& r : _style.rows) {
            if (r.unit == GridUnit::GROW) {
                _rows.pushBack({_Dim{row, growUnitRows * r.factor}});
                row += growUnitRows * r.factor;
            } else {
                _rows.pushBack({_Dim{row, r.size}});
                row += r.size;
            }

            row += _style.gaps.y;
        }

        _columns.clear();
        Au growUnitColumns = computeGrowUnitColumns(r);
        Au column = _style.flow.getStart(r);
        for (auto& c : _style.columns) {
            if (c.unit == GridUnit::GROW) {
                _columns.pushBack({_Dim{column, growUnitColumns * c.factor}});
                column += growUnitColumns * c.factor;
            } else {
                _columns.pushBack({_Dim{column, c.size}});
                column += c.size;
            }

            column += _style.gaps.x;
        }

        // layout the children
        isize index = 0;
        for (auto& child : children()) {
            if (child.is<Cell>()) {
                auto& cell = child.expect<Cell>();
                auto start = cell.start();
                auto end = cell.end();
                place(child, start, end);
                index = end.y * _columns.len() + end.x;
            } else {
                isize row = index / _columns.len();
                isize column = index % _columns.len();

                place(child, {column, row});
            }
            index++;
        }
    }

    Math::Vec2Au size(Math::Vec2Au s, Hint hint) override {
        Au row = 0_au;
        bool rowGrow = false;
        Au growUnitRows = computeGrowUnitRows(Math::RectAu{0_au, s});
        for (auto& r : _style.rows) {
            if (r.unit == GridUnit::GROW) {
                row += growUnitRows * r.factor;
                rowGrow = true;
            } else {
                row += r.size;
            }
        }

        row += computeGapsRows();

        if (rowGrow and hint == Hint::MAX) {
            row = max(_style.flow.getY(s), row);
        }

        Au column = 0_au;
        bool columnGrow = false;
        Au growUnitColumns = computeGrowUnitColumns(Math::RectAu{0_au, s});
        for (auto& c : _style.columns) {
            if (c.unit == GridUnit::GROW) {
                column += growUnitColumns * c.factor;
                columnGrow = true;
            } else {
                column += c.size;
            }
        }

        column += computeGapsColumns();

        if (columnGrow and hint == Hint::MAX) {
            column = max(_style.flow.getX(s), column);
        }

        return Math::Vec2Au{column, row};
    }
};

export Child grid(GridStyle style, Children children) {
    return makeRc<GridLayout>(style, children);
}

export Child grid(GridStyle style, Meta::Convertible<Child> auto... children) {
    return grid(style, Children{children...});
}

} // namespace Karm::Ui
