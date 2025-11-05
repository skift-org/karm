export module Karm.Vte:terminal;

import Karm.Font;
import Karm.Ref;
import :buffer;
import :parser;

namespace Karm::Vte {

struct Metrics {
    f64 baseline;
    f64 descend;
    f64 advance;
};

export struct ColorScheme {
    String name;
    Array<Gfx::Color, 16> colors;

    static ColorScheme light() {
        return {
            "System Light"s,
            {
                Gfx::ZINC50,
                Gfx::ZINC400,
                Gfx::ZINC500,
                Gfx::ZINC600,
                Gfx::ZINC700,
                Gfx::ZINC800,
                Gfx::ZINC900,
                Gfx::ZINC950,

                Gfx::YELLOW,
                Gfx::ORANGE,
                Gfx::RED,
                Gfx::PINK,
                Gfx::INDIGO,
                Gfx::BLUE,
                Gfx::CYAN,
                Gfx::GREEN,
            }
        };
    }

    static ColorScheme dark() {
        return {
            "System Dark"s,
            {
                Gfx::ZINC950,
                Gfx::ZINC900,
                Gfx::ZINC800,
                Gfx::ZINC700,
                Gfx::ZINC600,
                Gfx::ZINC500,
                Gfx::ZINC400,
                Gfx::ZINC50,

                Gfx::YELLOW,
                Gfx::ORANGE,
                Gfx::RED,
                Gfx::FUCHSIA,
                Gfx::INDIGO,
                Gfx::BLUE,
                Gfx::CYAN,
                Gfx::GREEN,
            }
        };
    }

    static ColorScheme solarized() {
        return {
            "Solarized"s,
            {
                Gfx::Color::fromHex(0x002b36),
                Gfx::Color::fromHex(0x073642),
                Gfx::Color::fromHex(0x586e75),
                Gfx::Color::fromHex(0x657b83),
                Gfx::Color::fromHex(0x839496),
                Gfx::Color::fromHex(0x93a1a1),
                Gfx::Color::fromHex(0xeee8d5),
                Gfx::Color::fromHex(0xfdf6e3),

                Gfx::Color::fromHex(0xb58900),
                Gfx::Color::fromHex(0xcb4b16),
                Gfx::Color::fromHex(0xdc322f),
                Gfx::Color::fromHex(0xd33682),
                Gfx::Color::fromHex(0x6c71c4),
                Gfx::Color::fromHex(0x268bd2),
                Gfx::Color::fromHex(0x2aa198),
                Gfx::Color::fromHex(0x859900),
            }
        };
    }

    static ColorScheme dracula() {
        return {
            "Dracula"s,
            {
                Gfx::Color::fromHex(0x282a36),
                Gfx::Color::fromHex(0x44475a),
                Gfx::Color::fromHex(0xf8f8f2),
                Gfx::Color::fromHex(0x6272a4),
                Gfx::Color::fromHex(0x8be9fd),
                Gfx::Color::fromHex(0x50fa7b),
                Gfx::Color::fromHex(0xffb86c),
                Gfx::Color::fromHex(0xff79c6),
                Gfx::Color::fromHex(0xbd93f9),
                Gfx::Color::fromHex(0xff5555),
                Gfx::Color::fromHex(0xf1fa8c),
                Gfx::Color::fromHex(0x21222c),
                Gfx::Color::fromHex(0x191a21),
                Gfx::Color::fromHex(0x6272a4),
                Gfx::Color::fromHex(0x44475a),
                Gfx::Color::fromHex(0x282a36),
            }
        };
    }

    static ColorScheme nord() {
        return {
            "Nord"s,
            {
                Gfx::Color::fromHex(0x2e3440),
                Gfx::Color::fromHex(0x3b4252),
                Gfx::Color::fromHex(0x434c5e),
                Gfx::Color::fromHex(0x4c566a),
                Gfx::Color::fromHex(0xd8dee9),
                Gfx::Color::fromHex(0xe5e9f0),
                Gfx::Color::fromHex(0xeceff4),
                Gfx::Color::fromHex(0x8fbcbb),
                Gfx::Color::fromHex(0x88c0d0),
                Gfx::Color::fromHex(0x81a1c1),
                Gfx::Color::fromHex(0x5e81ac),
                Gfx::Color::fromHex(0xbf616a),
                Gfx::Color::fromHex(0xd08770),
                Gfx::Color::fromHex(0xebcb8b),
                Gfx::Color::fromHex(0xa3be8c),
                Gfx::Color::fromHex(0xb48ead),
            }
        };
    }
};

export struct Theme {
    Gfx::Font font = {
        Font::loadFontfaceOrFallback("bundle://fonts-fira-code/fonts/FiraCode-Regular.ttf"_url).unwrap(),
        16,
        1.2,
    };

    Metrics metrics() {
        auto metrics = font.metrics();

        return {
            metrics.ascend + metrics.linegap,
            metrics.descend,
            font.advance(Gfx::Glyph::TOFU)
        };
    }
};

export struct Terminal {
    Theme _theme;
    Buffer _buffer;
    Parser _parser;
    Metrics _metrics;
    Attrs _attrs{
        .fg = Gfx::WHITE,
        .bg = Gfx::ALPHA,
    };
    Math::Vec2i _viewport;

    Terminal(Theme theme) : _theme(theme), _metrics(theme.metrics()) {}

    void paint(Gfx::Canvas& g) {
        g.push();
        isize cellHeight = Math::ceil(_metrics.baseline + _metrics.descend);
        isize cellWidth = Math::ceil(_metrics.advance);

        isize li = 0;
        for (auto& l : _buffer.lines) {
            isize ci = 0;
            auto lineY = li * cellHeight;
            if (l.separator)
                g.plot(Math::Edgei{0, lineY, _viewport.width, lineY}, Gfx::ZINC800);

            for (auto& c : l.cells) {
                Math::Recti cellBound = {
                    ci * cellWidth,
                    li * cellHeight,
                    cellWidth,
                    cellHeight,
                };

                g.fillStyle(c.attrs.bg);
                g.fill(cellBound);

                Math::Vec2i baseline = cellBound.topStart() + Math::Vec2i{0, (isize)_metrics.baseline};
                g.fillStyle(c.attrs.fg);
                g.fill(
                    _theme.font,
                    _theme.font.glyph(c.rune),
                    baseline.cast<f64>()
                );
                ci++;
            }
            li++;
        }

        g.pop();

        Math::Recti cellBound = {
            (isize)_buffer.cursor.x * cellWidth,
            (isize)_buffer.cursor.y * cellHeight,
            cellWidth,
            cellHeight,
        };

        g.push();
        g.fillStyle(Gfx::BLUE);
        g.fill(cellBound);
        g.pop();
    }

    void updateViewport(Math::Vec2i viewport) {
        _viewport = viewport;
    }

    void write(Rune rune) {
        if (rune == '\n') {
            _buffer.newline();
            return;
        } else if (rune == '\t') {
            _buffer.separator();
        } else {
            _buffer.append(rune, _attrs);
        }
    }

    void write(Str data) {
        for (auto r : iterRunes(data)) {
            write(r);
        }
    }
};

} // namespace Karm::Vte