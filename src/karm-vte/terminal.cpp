export module Karm.Vte:terminal;

import Karm.Font;
import Karm.Ref;
import Karm.Logger;
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
        12,
        1.2,
    };

    Gfx::Font boldFont = {
        Font::loadFontfaceOrFallback("bundle://fonts-fira-code/fonts/FiraCode-Bold.ttf"_url).unwrap(),
        12,
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

Gfx::Color bright(Gfx::Color color) {
    auto hsv = Gfx::rgbToHsv(color);
    hsv.value = clamp(hsv.value + 0.2f, 0.0f, 1.0f);
    return Gfx::hsvToRgb(hsv);
}

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
    isize _scrollTop = 0;

    Terminal(Theme theme) : _theme(theme), _metrics(theme.metrics()) {}

    void paint(Gfx::Canvas& g) {
        g.push();
        isize cellHeight = Math::ceil(_metrics.baseline + _metrics.descend);
        isize cellWidth = Math::ceil(_metrics.advance);

        isize visibleLines = _viewport.height / cellHeight;

        for (isize i = _scrollTop; i < _scrollTop + visibleLines + 1; ++i) {
            if (i >= (isize)_buffer.lines.len())
                break;

            auto& l = _buffer.lines[i];

            auto lineY = (i - _scrollTop) * cellHeight;

            if (l.separator)
                g.plot(Math::Edgei{0, lineY, _viewport.width, lineY}, Gfx::ZINC800);

            isize ci = 0;
            for (auto& c : l.cells) {
                Math::Recti cellBound = {
                    ci * cellWidth,
                    lineY,
                    cellWidth,
                    cellHeight,
                };

                g.fillStyle(c.attrs.bg);
                g.fill(cellBound);

                Math::Vec2i baseline = cellBound.topStart() + Math::Vec2i{0, (isize)_metrics.baseline};
                g.fillStyle(c.attrs.fg);
                if (c.attrs.bold) {
                    g.fill(
                        _theme.boldFont,
                        _theme.boldFont.glyph(c.rune),
                        baseline.cast<f64>()
                    );
                } else {
                    g.fill(
                        _theme.font,
                        _theme.font.glyph(c.rune),
                        baseline.cast<f64>()
                    );
                }
                ci++;
            }
        }

        g.pop();

        if (_buffer.cursor.y >= (usize)_scrollTop and _buffer.cursor.y < (usize)(_scrollTop + visibleLines)) {
            Math::Recti cellBound = {
                (isize)_buffer.cursor.x * cellWidth,
                (isize)(_buffer.cursor.y - _scrollTop) * cellHeight,
                cellWidth,
                cellHeight,
            };

            g.push();
            g.fillStyle(Gfx::BLUE);
            g.fill(cellBound);
            g.pop();
        }
    }

    void updateViewport(Math::Vec2i viewport) {
        _viewport = viewport;
    }

    void scroll(isize delta) {
        isize cellHeight = Math::ceil(_metrics.baseline + _metrics.descend);
        isize visibleLines = _viewport.height / cellHeight;
        isize totalLines = _buffer.lines.len();

        _scrollTop = clamp(_scrollTop + delta, 0, max(0, totalLines - visibleLines));
    }

    void scrollToBottom() {
        isize cellHeight = Math::ceil(_metrics.baseline + _metrics.descend);
        isize visibleLines = _viewport.height / cellHeight;

        if (_buffer.cursor.y >= (usize)(_scrollTop + visibleLines)) {
            _scrollTop = _buffer.cursor.y - visibleLines + 1;
        }
    }

    void _handleCsi(u8 b, Slice<usize> params) {
        usize n = params.len() > 0 ? params[0] : 0;
        switch (b) {

        case 'A':
            _buffer.moveCursorRelative({0, -(isize)n});
            break;

        case 'B':
            _buffer.moveCursorRelative({0, (isize)n});
            break;

        case 'C':
            _buffer.moveCursorRelative({(isize)n, 0});
            break;

        case 'D':
            _buffer.moveCursorRelative({-(isize)n, 0});
            break;

        case 'G':
            if (params.len() == 1) {
                _buffer.moveCursorToH(n - 1);
            }
            break;

        case 'H':
            if (params.len() == 2) {
                _buffer.moveCursorTo({params[0] - 1, params[1] - 1});
            }
            break;

        case 'J':
            if (n == 2) {
                _buffer.clearAll();
                _scrollTop = 0;
            } else {
                yap("unsupported CSI J {}", params);
            }
            break;

        case 'K':
            if (n == 0) {
                _buffer.clearAfterCursor();
            }
            break;

        case 'm':
            if (params.len() == 0) {
                _attrs = {
                    .fg = Gfx::WHITE,
                    .bg = Gfx::ALPHA,
                    .bold = false
                };
            }
            for (auto p : params) {
                if (p == 0)
                    _attrs = {
                        .fg = Gfx::WHITE,
                        .bg = Gfx::ALPHA,
                        .bold = false
                    };
                else if (p == 1) {
                    _attrs.bold = true;
                } else if (p >= 30 and p <= 37) {
                    _attrs.fg = ColorScheme::dark().colors[p - 30 + 8];
                } else if (p == 39) {
                    _attrs.fg = Gfx::WHITE;
                } else if (p == 49) {
                    _attrs.bg = Gfx::ALPHA;
                } else if (p >= 90 and p <= 97) {
                    _attrs.fg = bright(ColorScheme::dark().colors[p - 90 + 8]);
                } else if (p >= 40 and p <= 47) {
                    _attrs.bg = ColorScheme::dark().colors[p - 40 + 8];
                } else if (p >= 100 and p <= 107) {
                    _attrs.bg = bright(ColorScheme::dark().colors[p - 100 + 8]);
                } else {
                    logWarn("unsupported csi m {}", p);
                }
            }
            break;

        default:
            logWarn("unsupported csi {:c} {}", b, params);
        }
    }

    void write(Bytes buf) {
        for (auto& b : buf) {
            _parser.injest(b, [&](Parser::Action act, u8 b) {
                if (act == Parser::Action::PRINT) {
                    // FIXME: Do utf-8 decoding
                    _buffer.append(b, _attrs);
                } else if (act == Parser::Action::EXECUTE) {
                    if (b == '\n') {
                        _buffer.newline();
                    } else if (b == '\b') {
                        _buffer.backspace();
                    }
                } else if (act == Parser::Action::CSI_DISPATCH) {
                    _handleCsi(b, _parser.params());
                }
            });
        }

        scrollToBottom();
    }
};

} // namespace Karm::Vte