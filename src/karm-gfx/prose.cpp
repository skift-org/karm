export module Karm.Gfx:prose;

import Karm.Math;

import :color;
import :font;

using namespace Karm::Math::Literals;

namespace Karm::Gfx {

export enum struct TextAlign {
    LEFT,
    CENTER,
    RIGHT,
};

export struct ProseStyle {
    TextAlign align = TextAlign::LEFT;
    bool collapseEmptyLines = true;
    bool multiline = false;
};

export struct SpanStyle {
    Font font = Font::fallback();
    Opt<Color> color = NONE;
    Math::Au marginLeft = 0_au;
    Math::Au marginRight = 0_au;
    bool wordwrap = true;

    SpanStyle& withFont(Font const& value) {
        font = value;
        return *this;
    }

    SpanStyle& withColor(Color value) {
        color = value;
        return *this;
    }

    SpanStyle& withMarginLeft(Math::Au value) {
        marginLeft = value;
        return *this;
    }

    SpanStyle& withMarginRight(Math::Au value) {
        marginRight = value;
        return *this;
    }

    SpanStyle& withWordwrap(bool value) {
        wordwrap = value;
        return *this;
    }
};

export struct ProseProps :
    ProseStyle,
    SpanStyle {

    ProseProps() = default;

    ProseProps(Gfx::Font font) {
        this->font = font;
    }

    ProseProps(Rc<Gfx::Fontface> fontface) {
        this->font.fontface = fontface;
    }

    ProseProps& withAlign(Gfx::TextAlign value) {
        align = value;
        return *this;
    }

    ProseProps& withCollapseEmptyLines(bool value) {
        collapseEmptyLines = value;
        return *this;
    }

    ProseProps& withMultiline(bool value) {
        multiline = value;
        return *this;
    }

    ProseProps& withFont(Gfx::Font const& value) {
        font = value;
        return *this;
    }

    ProseProps& withFontSize(f64 fontsize) {
        font.fontsize = fontsize;
        return *this;
    }

    ProseProps& withColor(Gfx::Color value) {
        color = value;
        return *this;
    }

    ProseProps& withMarginLeft(Math::Au value) {
        marginLeft = value;
        return *this;
    }

    ProseProps& withMarginRight(Math::Au value) {
        marginRight = value;
        return *this;
    }

    ProseProps& withWordwrap(bool value) {
        wordwrap = value;
        return *this;
    }
};

export struct Prose : Meta::Pinned {
    struct Span {
        Opt<Rc<Span>> parent;
        SpanStyle style;

        void repr(Io::Emit& e) const {
            e("(span color={} margins=({}, {}))", style.color, style.marginLeft, style.marginRight);
        }
    };

    struct StrutCell {
        usize id;
        Math::Vec2Au size{};
        // NOTE: baseline is distance from strut's top to the considered baseline
        Math::Au baseline{};

        void repr(Io::Emit& e) const {
            e("(StrutCell id: {} size: {} baseline: {})", id, size, baseline);
        }
    };

    enum struct CellType {
        GLYPH,
        SPACER,
        STRUT,
    };

    struct Cell {
        Opt<Prose&> prose;
        Rc<Span> span;

        urange runeRange;
        Glyph glyph;
        Math::Au pos = 0_au; //< Position of the glyph within the block
        Math::Au adv = 0_au; //< Advance of the glyph

        static constexpr u32 GLYPH_SENTINEL = -1;
        static constexpr u32 SPACER_SENTINEL = -2;

        // Encodes the cell type and strut index, see type() and strutIndex() for more infos.
        u32 extra = GLYPH_SENTINEL;

        CellType type() const {
            if (extra == GLYPH_SENTINEL) {
                return CellType::GLYPH;
            } else if (extra == SPACER_SENTINEL) {
                return CellType::SPACER;
            } else {
                return CellType::STRUT;
            }
        }

        usize strutIndex() const {
            if (type() != CellType::STRUT)
                panic("cell is not a strut");

            return extra;
        }

        SpanStyle const& style() const {
            return span->style;
        }

        void measureAdvance() {
            if (type() == CellType::SPACER) {
                // Nothing because adv is already set.
            } else if (type() == CellType::STRUT) {
                adv = strut()->size.x;
            } else {
                adv = Math::Au{style().font.advance(glyph)};
            }
        }

        MutSlice<Rune> runes() {
            return mutSub(prose->_runes, runeRange);
        }

        Slice<Rune> runes() const {
            return sub(prose->_runes, runeRange);
        }

        bool newline() const {
            auto r = runes();
            if (not r)
                return false;
            return last(r) == '\n';
        }

        bool space() const {
            auto r = runes();
            if (not r)
                return false;
            return last(r) == '\n' or isAsciiSpace(last(r));
        }

        Cursor<StrutCell> strut() const {
            if (type() != CellType::STRUT)
                return nullptr;

            return &prose->_struts[strutIndex()];
        }

        MutCursor<StrutCell> strut() {
            if (type() != CellType::STRUT)
                return nullptr;

            return &prose->_struts[strutIndex()];
        }

        Math::Au yPosition(Math::Au dominantBaselineYPosition) const {
            if (type() != CellType::STRUT)
                return dominantBaselineYPosition;

            return dominantBaselineYPosition - strut()->baseline;
        }
    };

    struct Block {
        Opt<Prose&> prose;

        urange runeRange;
        urange cellRange;

        Math::Au pos = 0_au; // Position of the block within the line
        Math::Au width = 0_au;

        MutSlice<Cell> cells() {
            return mutSub(prose->_cells, cellRange);
        }

        Slice<Cell> cells() const {
            return sub(prose->_cells, cellRange);
        }

        bool empty() const {
            return cellRange.empty();
        }

        bool spaces() const {
            if (empty())
                return false;
            return last(cells()).space();
        }

        bool newline() const {
            if (empty())
                return false;
            return last(cells()).newline();
        }

        MutCursor<StrutCell> strut() {
            if (empty())
                return nullptr;

            auto cellsRef = cells();
            return last(cellsRef).strut();
        }

        Cursor<StrutCell> strut() const {
            if (empty())
                return nullptr;

            return last(cells()).strut();
        }

        bool allowsBreakAfter() const {
            if (empty())
                return false;
            if (strut())
                return true;
            return last(cells()).style().wordwrap;
        }

        bool forcesBreakAfter() const {
            if (empty())
                return false;
            return newline() and prose->_style.multiline;
        }
    };

    struct Line {
        Opt<Prose&> prose;

        urange runeRange;
        urange blockRange;
        Math::Au baseline = 0_au; // Baseline of the line within the text
        Math::Au width = 0_au;
        Math::Au ascent = 0_au;
        Math::Au descend = 0_au;

        Slice<Block> blocks() const {
            return sub(prose->_blocks, blockRange);
        }

        MutSlice<Block> blocks() {
            return mutSub(prose->_blocks, blockRange);
        }
    };

    ProseStyle _style;

    Vec<Rune> _runes;
    Vec<Cell> _cells;
    Vec<Block> _blocks;
    Vec<Line> _lines;

    Vec<StrutCell> _struts;
    Vec<usize> _strutCellsIndexes;

    // Various cached values
    bool _blocksMeasured = false;

    Math::Vec2Au _size;

    Prose(ProseProps props, Str str = "") : Prose(props, props, str) {}

    Prose(ProseStyle style, SpanStyle rootSpanStyle, Str str = "")
        : _style(style),
          _currentSpan(makeRc<Span>(NONE, rootSpanStyle)),
          _rootSpan(_currentSpan) {
        clear();
        _spanHistory.pushBack(_rootSpan);
        append(str);
    }

    static Vec<Rc<Span const>> _ancestorChain(Rc<Span const> span) {
        Vec<Rc<Span const>> chain;
        Opt<Rc<Span const>> cur = span;
        while (cur) {
            chain.pushBack(cur.unwrap());
            cur = cur.unwrap()->parent;
        }
        reverse(mutSub(chain));
        return chain;
    }

    static Rc<Span> _findRoot(Rc<Span const> span) {
        while (span->parent)
            span = span->parent.unwrap();
        return span;
    }

    Prose(ProseStyle style, Rc<Span const> continueFrom)
        : _style(style),
          _currentSpan(continueFrom),
          _rootSpan(_findRoot(continueFrom)),
          _spanHistory(_ancestorChain(continueFrom)) {
        clear();
    }

    Math::Vec2Au size() const {
        return _size;
    }

    // MARK: Prose --------------------------------------------------------------

    void _beginBlock() {
        _blocks.pushBack({
            .prose = *this,
            .runeRange = {_runes.len(), 0},
            .cellRange = {_cells.len(), 0},
        });
    }

    void clear() {
        _runes.clear();
        _cells.clear();
        _blocks.clear();
        _blocksMeasured = false;
        _lines.clear();

        _beginBlock();
    }

    void append(Rune rune) {
        if (any(_blocks) and (last(_blocks).newline() or last(_blocks).spaces() or last(_blocks).strut()))
            _beginBlock();

        auto glyph = _currentSpan->style.font.glyph(rune == '\n' ? ' ' : rune);

        _cells.pushBack({
            .prose = *this,
            .span = _currentSpan,
            .runeRange = {_runes.len(), 1},
            .glyph = glyph,
            .extra = Cell::GLYPH_SENTINEL,
        });

        _runes.pushBack(rune);
        last(_blocks).cellRange.size++;
        last(_blocks).runeRange.end(_runes.len());
    }

    void appendSpacer(Math::Au width) {
        _cells.pushBack({
            .prose = *this,
            .span = _currentSpan,
            .runeRange = {_runes.len(), 1},
            .glyph = Glyph::TOFU,
            .adv = width,
            .extra = Cell::SPACER_SENTINEL,
        });

        _runes.pushBack(0);

        last(_blocks).cellRange.size++;
        last(_blocks).runeRange.end(_runes.len());
    }

    void append(Slice<Rune> runes) {
        _runes.ensure(_runes.len() + runes.len());
        for (auto rune : runes) {
            append(rune);
        }
    }

    template <typename E>
    void append(_Str<E> str) {
        for (auto rune : iterRunes(str))
            append(rune);
    }

    template <typename E>
    void append(_String<E> const& str) {
        append(str.str());
    }

    void append(StrutCell&& strut) {
        if (_blocks.len() and not last(_blocks).empty())
            _beginBlock();

        _strutCellsIndexes.pushBack(_cells.len());
        _cells.pushBack({
            .prose = *this,
            .span = _currentSpan,
            .runeRange = {_runes.len(), 1},
            .glyph = Glyph::TOFU,
            .extra = static_cast<u32>(_struts.len()),
        });
        _struts.pushBack(std::move(strut));

        _runes.pushBack(0);

        last(_blocks).cellRange.size++;
        last(_blocks).runeRange.end(_runes.len());
    }

    // MARK: Span --------------------------------------------------------------

    Rc<Span const> _currentSpan;
    Rc<Span const> _rootSpan;

    // INFO: Used for testing, might be removed later.
    Vec<Rc<Span const>> _spanHistory{};

    void pushSpan(SpanStyle const& spanStyle) {
        auto span = makeRc<Span>(_currentSpan, spanStyle);

        _spanHistory.pushBack(span);

        auto refToLast = span;
        _currentSpan = refToLast;

        if (spanStyle.marginLeft != 0_au)
            appendSpacer(spanStyle.marginLeft);
    }

    void popSpan() {
        if (not _currentSpan->parent)
            panic("popping the root span");

        if (_currentSpan->style.marginRight != 0_au)
            appendSpacer(_currentSpan->style.marginRight);

        auto newCurr = _currentSpan->parent.unwrap();
        _currentSpan = newCurr;
    }

    SpanStyle currentSpanStyle() const {
        return _currentSpan->style;
    }

    // MARK: Strut ------------------------------------------------------------

    Vec<MutCursor<Cell>> cellsWithStruts() {
        // FIXME: Vec of MutCursor of length 1 is bad design, try to use Generator
        Vec<MutCursor<Cell>> cells;
        for (auto i : _strutCellsIndexes)
            cells.pushBack(&_cells[i]);
        return cells;
    }

    // MARK: Layout ------------------------------------------------------------

    void _measureBlocks() {
        for (auto& block : _blocks) {
            auto adv = 0_au;
            bool first = true;
            Glyph prev = Glyph::TOFU;
            for (auto& cell : block.cells()) {
                if (not first)
                    adv += Math::Au{cell.style().font.kern(prev, cell.glyph)};
                else
                    first = false;

                cell.pos = adv;
                cell.measureAdvance();

                adv += cell.adv;
                prev = cell.glyph;
            }
            block.width = adv;
        }
    }

    void _wrapLines(Math::Au width) {
        _lines.clear();

        Line line{*this, {}, {}};
        bool first = true;
        Math::Au adv = 0_au;
        for (usize i = 0; i < _blocks.len(); i++) {
            auto& block = _blocks[i];

            bool wordwrapAllowed = not first and _blocks[i - 1].allowsBreakAfter();

            if (adv + block.width > width and wordwrapAllowed) {
                _lines.pushBack(line);
                line = {*this, block.runeRange, {i, 1}};
                adv = block.width;

                if (block.forcesBreakAfter()) {
                    _lines.pushBack(line);
                    line = {*this, {block.runeRange.end(), 0}, {i + 1, 0}};
                    adv = 0_au;
                }
            } else {
                line.blockRange.size++;
                line.runeRange.end(block.runeRange.end());

                if (block.forcesBreakAfter()) {
                    _lines.pushBack(line);
                    line = {*this, {block.runeRange.end(), 0}, {i + 1, 0}};
                    adv = 0_au;
                } else {
                    adv += block.width;
                }
            }
            first = false;
        }

        _lines.pushBack(line);
    }

    Math::Au _layoutVerticaly() {
        auto m = _rootSpan->style.font.metrics();

        // NOTE: applying ceiling so fonts are pixel aligned
        f64 halfFontLineGap = m.linegap / 2;
        Math::Au fontAscent = Math::Au{Math::ceil(m.ascend + halfFontLineGap)};
        Math::Au fontDescend = Math::Au{Math::ceil(m.descend + halfFontLineGap)};

        Math::Au currHeight = 0_au;
        for (auto& line : _lines) {
            Math::Au lineTop = currHeight;

            Math::Au maxAscent = 0_au;
            Math::Au maxDescend = 0_au;

            if (not _style.collapseEmptyLines) {
                maxAscent = fontAscent;
                maxDescend = fontDescend;
            }

            for (auto const& block : line.blocks()) {
                if (block.strut()) {
                    Math::Au baseline{block.strut()->baseline};
                    maxAscent = max(maxAscent, baseline);
                    maxDescend = max(maxDescend, block.strut()->size.y - baseline);
                } else {
                    maxAscent = max(maxAscent, fontAscent);
                    maxDescend = max(maxDescend, fontDescend);
                }
            }

            line.baseline = lineTop + maxAscent;
            line.ascent = maxAscent;
            line.descend = maxDescend;
            currHeight += maxAscent + maxDescend;
        }

        return currHeight;
    }

    Math::Au _layoutHorizontaly(Math::Au width) {
        Math::Au maxWidth = 0_au;
        for (auto& line : _lines) {
            if (not line.blockRange.any())
                continue;

            Math::Au pos = 0_au;
            for (auto& block : line.blocks()) {
                block.pos = pos;
                pos += block.width;
            }

            auto lastBlock = _blocks[line.blockRange.end() - 1];
            line.width = lastBlock.pos + lastBlock.width;
            maxWidth = max(maxWidth, line.width);
            auto free = width - line.width;

            switch (_style.align) {
            case TextAlign::LEFT:
                break;

            case TextAlign::CENTER:
                for (auto& block : line.blocks())
                    block.pos += free / 2;
                break;

            case TextAlign::RIGHT:
                for (auto& block : line.blocks())
                    block.pos += free;
                break;
            }
        }

        return maxWidth;
    }

    Math::Vec2Au layout(Math::Au width) {
        if (isEmpty(_blocks))
            return {};

        // Blocks measurements can be reused between layouts changes
        // only line wrapping need to be re-done
        if (not _blocksMeasured) {
            _measureBlocks();
            _blocksMeasured = true;
        }

        _wrapLines(width);
        auto textHeight = _layoutVerticaly();
        auto textWidth = _layoutHorizontaly(width);
        _size = {textWidth, textHeight};
        return {textWidth, textHeight};
    }

    // MARK: Paint -------------------------------------------------------------

    struct Lbc {
        usize li, bi, ci;
    };

    Lbc lbcAt(usize runeIndex) const {
        auto maybeLi = searchLowerBound(
            _lines, [&](Line const& l) {
                return l.runeRange.start <=> runeIndex;
            }
        );

        if (not maybeLi)
            return {0, 0, 0};

        auto li = *maybeLi;

        auto& line = _lines[li];

        auto maybeBi = searchLowerBound(
            line.blocks(), [&](Block const& b) {
                return b.runeRange.start <=> runeIndex;
            }
        );

        if (not maybeBi)
            return {li, 0, 0};

        auto bi = *maybeBi;

        auto& block = line.blocks()[bi];

        auto maybeCi = searchLowerBound(
            block.cells(), [&](Cell const& c) {
                return c.runeRange.start <=> runeIndex;
            }
        );

        if (not maybeCi)
            return {li, bi, 0};

        auto ci = *maybeCi;

        auto cell = block.cells()[ci];

        if (cell.runeRange.end() == runeIndex) {
            // Handle the case where the rune is the last of the text
            ci++;
        }

        return {li, bi, ci};
    }

    Tuple<Math::Vec2Au, Math::Au, Math::Au> queryCaret(usize runeIndex) const {
        auto [li, bi, ci] = lbcAt(runeIndex);

        if (isEmpty(_lines))
            return {};

        auto& line = _lines[li];

        if (isEmpty(line.blocks()))
            return {
                {
                    0_au,
                    line.baseline,
                },
                line.ascent,
                line.descend,
            };

        auto& block = line.blocks()[bi];

        if (isEmpty(block.cells()))
            return {
                {
                    block.pos,
                    line.baseline,
                },
                line.ascent,
                line.descend,
            };

        if (ci >= block.cells().len()) {
            // Handle the case where the rune is the last of the text
            auto& cell = last(block.cells());
            return {
                {
                    block.pos + cell.pos + cell.adv,
                    cell.yPosition(line.baseline),
                },
                line.ascent,
                line.descend,
            };
        }

        auto& cell = block.cells()[ci];
        return {
            Math::Vec2Au{
                block.pos + cell.pos,
                cell.yPosition(line.baseline),
            },
            line.ascent,
            line.descend
        };
    }

    Math::Vec2Au
    queryPosition(usize runeIndex) const {
        auto [pos, _, _] = queryCaret(runeIndex);
        return pos;
    }

    // MARK: Hit Testing -------------------------------------------------------

    usize hitTest(Math::Vec2Au point) const {
        if (isEmpty(_lines))
            return 0;

        // Find the line containing the y coordinate
        usize li = _lines.len() - 1;
        for (usize i = 0; i + 1 < _lines.len(); i++) {
            Math::Au nextLineTop = _lines[i + 1].baseline - _lines[i + 1].ascent;
            if (point.y < nextLineTop) {
                li = i;
                break;
            }
        }

        auto& line = _lines[li];

        if (isEmpty(line.blocks()))
            return line.runeRange.start;

        // Find the block closest to the x coordinate
        usize bi = 0;
        for (usize i = 0; i < line.blocks().len(); i++) {
            auto& block = line.blocks()[i];
            if (point.x < block.pos + block.width) {
                bi = i;
                break;
            }
            bi = i;
        }

        auto& block = line.blocks()[bi];

        if (isEmpty(block.cells()))
            return block.runeRange.start;

        // Find the cell closest to the x coordinate
        for (auto& cell : block.cells()) {
            Math::Au cellMid = block.pos + cell.pos + cell.adv / 2;
            if (point.x < cellMid)
                return cell.runeRange.start;
        }

        // Past the last cell — return end of block
        auto& lastCell = last(block.cells());
        return lastCell.runeRange.end();
    }

    void repr(Io::Emit& e) const {
        e("(prose)");
    }
};

} // namespace Karm::Gfx
