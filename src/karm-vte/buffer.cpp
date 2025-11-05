export module Karm.Vte:buffer;

import Karm.Core;
import Karm.Font;
import Karm.Gfx;
import Karm.Math;

namespace Karm::Vte {

struct Attrs {
    Gfx::Color fg;
    Gfx::Color bg;
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
    Math::Vec2u size;
    Vec<Line> lines;
    Math::Vec2u cursor;

    Line& line(usize at) {
        if (at >= lines.len())
            lines.resize(at + 1);
        return lines[at];
    }

    void append(Rune rune, Attrs attrs) {
        line(cursor.y).append(cursor.x, rune, attrs);
        cursor.x++;
    }

    void separator() {
        line(cursor.y).separator = true;
    }

    void newline() {
        cursor.x = 0;
        cursor.y++;
    }
};

} // namespace Karm::Vte