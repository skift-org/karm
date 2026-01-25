export module Karm.Vte:buffer;

import Karm.Core;
import Karm.Font;
import Karm.Gfx;
import Karm.Math;

namespace Karm::Vte {

struct Attrs {
    Gfx::Color fg;
    Gfx::Color bg;
    bool bold = false;
};

struct Cell {
    Rune rune;
    Attrs attrs;
};

struct Line {
    Vec<Cell> cells;
    bool separator = false;

    void append(usize x, Rune rune, Attrs attrs) {
        if (x >= cells.len())
            cells.resize(x + 1);
        cells[x].rune = rune;
        cells[x].attrs = attrs;
    }
};

struct Buffer {
    Math::Vec2u size = {9999};
    Vec<Line> lines;
    Math::Vec2u cursor;

    Line& line(usize at) {
        if (at >= lines.len())
            lines.resize(at + 1);
        return lines[at];
    }

    auto width() const {
        return size.width;
    }

    auto height() const {
        return size.height;
    }

    void append(Rune rune, Attrs attrs) {
        line(cursor.y).append(cursor.x, rune, attrs);
        cursor.x++;
    }

    void moveCursorRelative(Math::Vec2i off) {
        isize newX = static_cast<isize>(cursor.x) + off.x;
        isize newY = static_cast<isize>(cursor.y) + off.y;
        cursor.x = clamp(newX, 0, static_cast<isize>(size.x) - 1);
        cursor.y = clamp(newY, 0, static_cast<isize>(size.y) - 1);
    }

    void moveCursorTo(Math::Vec2u pos) {
        cursor.x = pos.x;
        cursor.y = pos.y;
    }

    void moveCursorToH(usize col) {
        cursor.x = col;
    }

    void separator() {
        line(cursor.y).separator = true;
    }

    void newline() {
        cursor.x = 0;
        cursor.y++;
    }

    void backspace() {
        if (cursor.x > 0) {
            cursor.x--;
        } else if (cursor.y > 0) {
            cursor.y--;
            cursor.x = size.x > 0 ? size.x - 1 : 0;
        }
    }

    void clearAll() {
        lines.clear();
        cursor = {0, 0};
    }

    void clearAfterCursor() {
        line(cursor.y).cells.resize(cursor.x);
    }
};

} // namespace Karm::Vte