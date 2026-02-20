export module Karm.Gfx:prose;

import Karm.Math;

import :color;
import :font;

namespace Karm::Gfx {

export enum struct TextAlign {
    LEFT,
    CENTER,
    RIGHT,
};

export struct ProseStyle {
    Font font;
    TextAlign align = TextAlign::LEFT;
    Opt<Color> color = NONE;
    bool wordwrap = true;
    bool multiline = false;
    bool collapseEmptyLines = true;

    ProseStyle withSize(f64 size) const {
        ProseStyle style = *this;
        style.font.fontsize = size;
        return style;
    }

    ProseStyle withLineHeight(f64 height) const {
        ProseStyle style = *this;
        style.font.lineheight = height;
        return style;
    }

    ProseStyle withAlign(TextAlign align) const {
        ProseStyle style = *this;
        style.align = align;
        return style;
    }

    ProseStyle withColor(Color color) const {
        ProseStyle style = *this;
        style.color = color;
        return style;
    }

    ProseStyle withWordwrap(bool wordwrap) const {
        ProseStyle style = *this;
        style.wordwrap = wordwrap;
        return style;
    }

    ProseStyle withMultiline(bool multiline) const {
        ProseStyle style = *this;
        style.multiline = multiline;
        return style;
    }
};

export struct Prose : Meta::Pinned {
    struct Span {
        Opt<Rc<Span>> parent;
        Opt<Color> color;
    };

    struct StrutCell {
        usize id;
        Vec2Au size{};
        // NOTE: baseline is distance from strut's top to the considered baseline
        Au baseline{};

        void repr(Io::Emit& e) const {
            e("(StrutCell id: {} size: {} baseline: {})", id, size, baseline);
        }
    };

    struct Cell {
        MutCursor<Prose> prose;
        Opt<Rc<Span>> span;

        urange runeRange;
        Glyph glyph;
        Au pos = 0_au; //< Position of the glyph within the block
        Au adv = 0_au; //< Advance of the glyph

        Opt<usize> relatedStrutIndex = NONE;

        void measureAdvance() {
            if (strut()) {
                adv = prose->_struts[relatedStrutIndex.unwrap()].size.x;
            } else {
                adv = Au{prose->_style.font.advance(glyph)};
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
            if (relatedStrutIndex == NONE)
                return nullptr;

            return &prose->_struts[relatedStrutIndex.unwrap()];
        }

        MutCursor<StrutCell> strut() {
            if (relatedStrutIndex == NONE)
                return nullptr;

            return &prose->_struts[relatedStrutIndex.unwrap()];
        }

        Au yPosition(Au dominantBaselineYPosition) const {
            if (relatedStrutIndex == NONE)
                return dominantBaselineYPosition;

            return dominantBaselineYPosition - prose->_struts[relatedStrutIndex.unwrap()].baseline;
        }
    };

    struct Block {
        MutCursor<Prose> prose;

        urange runeRange;
        urange cellRange;

        Au pos = 0_au; // Position of the block within the line
        Au width = 0_au;

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
    };

    struct Line {
        MutCursor<Prose> prose;

        urange runeRange;
        urange blockRange;
        Au baseline = 0_au; // Baseline of the line within the text
        Au width = 0_au;

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
    f64 _spaceWidth{};
    f64 _lineHeight{};

    Vec2Au _size;

    Prose(ProseStyle style, Str str = "") : _style(style) {
        clear();
        _spaceWidth = _style.font.advance(_style.font.glyph(' '));
        auto m = _style.font.metrics();
        _lineHeight = m.ascend;
        append(str);
    }

    Vec2Au size() const {
        return _size;
    }

    // MARK: Prose --------------------------------------------------------------

    void _beginBlock() {
        _blocks.pushBack({
            .prose = this,
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

        auto glyph = _style.font.glyph(rune == '\n' ? ' ' : rune);

        _cells.pushBack({
            .prose = this,
            .span = _currentSpan,
            .runeRange = {_runes.len(), 1},
            .glyph = glyph,
        });

        _runes.pushBack(rune);
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

    void append(StrutCell&& strut) {
        if (_blocks.len() and not last(_blocks).empty())
            _beginBlock();

        _strutCellsIndexes.pushBack(_cells.len());
        _cells.pushBack({
            .prose = this,
            .span = _currentSpan,
            .runeRange = {_runes.len(), 1},
            .glyph = Glyph::TOFU,
            .relatedStrutIndex = _struts.len(),
        });
        _struts.pushBack(std::move(strut));

        _runes.pushBack(0);

        last(_blocks).cellRange.size++;
        last(_blocks).runeRange.end(_runes.len());
    }

    // MARK: Span --------------------------------------------------------------

    Vec<Rc<Span>> _spans;
    Opt<Rc<Span>> _currentSpan = NONE;

    void pushSpan() {
        if (_currentSpan == NONE)
            _spans.pushBack(makeRc<Span>(Span{}));
        else
            _spans.pushBack(makeRc<Span>(_currentSpan->unwrap()));

        last(_spans)->parent = _currentSpan;

        auto refToLast = last(_spans);
        _currentSpan = refToLast;
    }

    void spanColor(Color color) {
        if (not _currentSpan)
            return;

        _currentSpan.unwrap()->color = color;
    }

    void popSpan() {
        if (not _currentSpan)
            return;

        auto newCurr = _currentSpan.unwrap()->parent;
        _currentSpan = newCurr;
    }

    void overrideSpanStackWith(Prose const& prose) {
        _spans.clear();

        for (auto currSpan = prose._currentSpan; currSpan != NONE; currSpan = currSpan.unwrap()->parent) {
            Rc<Span> copySpanRc = currSpan.unwrap();
            _spans.pushBack(copySpanRc);
        }

        reverse(mutSub(_spans));

        if (_spans.len()) {
            auto lastSpan = last(_spans);
            _currentSpan = lastSpan;
        }
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
                    adv += Au{_style.font.kern(prev, cell.glyph)};
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

    void _wrapLines(Au width) {
        _lines.clear();

        Line line{this, {}, {}};
        bool first = true;
        Au adv = 0_au;
        for (usize i = 0; i < _blocks.len(); i++) {
            auto& block = _blocks[i];
            if (adv + block.width > width and _style.wordwrap and _style.multiline and not first) {
                _lines.pushBack(line);
                line = {this, block.runeRange, {i, 1}};
                adv = block.width;

                if (block.newline()) {
                    _lines.pushBack(line);
                    line = {
                        this,
                        {block.runeRange.end(), 0},
                        {i + 1, 0},
                    };
                    adv = 0_au;
                }
            } else {
                line.blockRange.size++;
                line.runeRange.end(block.runeRange.end());

                if (block.newline() and _style.multiline) {
                    _lines.pushBack(line);
                    line = {
                        this,
                        {block.runeRange.end(), 0},
                        {i + 1, 0},
                    };
                    adv = 0_au;
                } else {
                    adv += block.width;
                }
            }
            first = false;
        }

        _lines.pushBack(line);
    }

    Au _layoutVerticaly() {
        auto m = _style.font.metrics();

        // NOTE: applying ceiling so fonts are pixel aligned
        f64 halfFontLineGap = m.linegap / 2;
        Au fontAscent = Au{Math::ceil(m.ascend + halfFontLineGap)};
        Au fontDescend = Au{Math::ceil(m.descend + halfFontLineGap)};

        Au currHeight = 0_au;
        for (auto& line : _lines) {
            Au lineTop = currHeight;

            Au maxAscent = 0_au;
            Au maxDescend = 0_au;

            if (not _style.collapseEmptyLines) {
                maxAscent = fontAscent;
                maxAscent = fontDescend;
            }

            for (auto const& block : line.blocks()) {
                if (block.strut()) {
                    Au baseline{block.strut()->baseline};
                    maxAscent = max(maxAscent, baseline);
                    maxDescend = max(maxDescend, block.strut()->size.y - baseline);
                } else {
                    maxAscent = max(maxAscent, fontAscent);
                    maxDescend = max(maxDescend, fontDescend);
                }
            }

            line.baseline = lineTop + maxAscent;
            currHeight += maxAscent + maxDescend;
        }

        return currHeight;
    }

    Au _layoutHorizontaly(Au width) {
        Au maxWidth = 0_au;
        for (auto& line : _lines) {
            if (not line.blockRange.any())
                continue;

            Au pos = 0_au;
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
                    block.pos += free / 2_au;
                break;

            case TextAlign::RIGHT:
                for (auto& block : line.blocks())
                    block.pos += free;
                break;
            }
        }

        return maxWidth;
    }

    Vec2Au layout(Au width) {
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

    Vec2Au queryPosition(usize runeIndex) const {
        auto [li, bi, ci] = lbcAt(runeIndex);

        if (isEmpty(_lines))
            return {};

        auto& line = _lines[li];

        if (isEmpty(line.blocks()))
            return {0_au, line.baseline};

        auto& block = line.blocks()[bi];

        if (isEmpty(block.cells()))
            return {block.pos, line.baseline};

        if (ci >= block.cells().len()) {
            // Handle the case where the rune is the last of the text
            auto& cell = last(block.cells());
            return {block.pos + cell.pos + cell.adv, cell.yPosition(line.baseline)};
        }

        auto& cell = block.cells()[ci];

        return {block.pos + cell.pos, cell.yPosition(line.baseline)};
    }

    // MARK: Hit Testing -------------------------------------------------------

    usize hitTest(Vec2Au point) const {
        if (isEmpty(_lines))
            return 0;

        Au fontAscent = Au{_lineHeight};

        // Find the line containing the y coordinate
        usize li = _lines.len() - 1;
        for (usize i = 0; i + 1 < _lines.len(); i++) {
            Au nextLineTop = _lines[i + 1].baseline - fontAscent;
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
            Au cellMid = block.pos + cell.pos + cell.adv / 2_au;
            if (point.x < cellMid)
                return cell.runeRange.start;
        }

        // Past the last cell — return end of block
        auto& lastCell = last(block.cells());
        return lastCell.runeRange.end();
    }
};

} // namespace Karm::Gfx
