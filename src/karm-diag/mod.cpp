module;

#include <karm/macros>

export module Karm.Diag;

import Karm.Core;
import Karm.Tty;
import Karm.Ref;

using namespace Karm::Literals;

namespace Karm::Diag {

static constexpr Tty::Style TTY_BOLD = {.bold = true};
static constexpr Tty::Style TTY_ACCENT = {.foreground = Tty::BLUE, .bold = true};
static constexpr Tty::Style TTY_ERROR = {.foreground = Tty::RED, .bold = true};
static constexpr Tty::Style TTY_WARNING = {.foreground = Tty::YELLOW, .bold = true};
static constexpr Tty::Style TTY_NOTE = {.foreground = Tty::CYAN, .bold = true};
static constexpr Tty::Style TTY_HELP = {.foreground = Tty::GREEN, .bold = true};

// MARK: Diagnostic Level ------------------------------------------------------

export enum struct Level {
    ERROR,
    WARNING,
    NOTE,
    HELP,

    _LEN,
};

static Tty::Style levelStyle(Level level) {
    switch (level) {
    case Level::ERROR:
        return TTY_ERROR;

    case Level::WARNING:
        return TTY_WARNING;

    case Level::NOTE:
        return TTY_NOTE;

    case Level::HELP:
        return TTY_HELP;

    default:
        return Tty::RESET;
    }
}

static Str levelName(Level level) {
    switch (level) {
    case Level::ERROR:
        return "error"s;
    case Level::WARNING:
        return "warning"s;
    case Level::NOTE:
        return "note"s;
    case Level::HELP:
        return "help"s;
    default:
        return "unknown"s;
    }
}

// MARK: Diagnostic Label ------------------------------------------------------

export struct Label {
    Io::LocSpan span;
    String message;
    bool isPrimary = true;

    static Label primary(Io::LocSpan span, Str message = ""s) {
        return Label{span, String{message}, true};
    }

    static Label secondary(Io::LocSpan span, Str message = ""s) {
        return Label{span, String{message}, false};
    }
};

// MARK: Diagnostic ------------------------------------------------------------

export struct Diagnostic {
    Level level = Level::ERROR;
    String code;
    String message;
    Vec<Label> labels;
    Vec<String> notes;
    Opt<String> help;

    static Diagnostic error(Str message) {
        return Diagnostic{Level::ERROR, ""s, String{message}, {}, {}, NONE};
    }

    static Diagnostic error(Str code, Str message) {
        return Diagnostic{Level::ERROR, String{code}, String{message}, {}, {}, NONE};
    }

    static Diagnostic warning(Str message) {
        return Diagnostic{Level::WARNING, ""s, String{message}, {}, {}, NONE};
    }

    Diagnostic& withLabel(Label label) {
        labels.pushBack(label);
        return *this;
    }

    Diagnostic& withPrimaryLabel(Io::LocSpan span, Str message = ""s) {
        labels.pushBack(Label::primary(span, message));
        return *this;
    }

    Diagnostic& withSecondaryLabel(Io::LocSpan span, Str message = ""s) {
        labels.pushBack(Label::secondary(span, message));
        return *this;
    }

    Diagnostic& withNote(Str note) {
        notes.pushBack(String{note});
        return *this;
    }

    Diagnostic& withHelp(Str helpText) {
        help = String{helpText};
        return *this;
    }
};

// MARK: Collector ------------------------------------------------------------

export struct Collector {
    Vec<Diagnostic> diags;
    bool _ignore = false;

    static Collector ignore() {
        return {{}, true};
    }

    void emit(Diagnostic d) {
        if (_ignore)
            return;
        diags.pushBack(std::move(d));
    }

    bool any() const {
        return diags.len();
    }

    void clear() {
        diags.clear();
    }
};

// MARK: Diagnostic Renderer ---------------------------------------------------

export struct Renderer {
    Str _source;
    Union<None, String, Ref::Url> _filename;

    Renderer(Str source, Union<None, String, Ref::Url> filename = NONE)
        : _source(source), _filename(filename) {}

    static void _writeSpaces(Io::Emit& e, usize count) {
        for (usize i = 0; i < count; i++)
            e(' ');
    }

    Str _lineContent(usize lineNum) const {
        usize currentLine = 1;
        usize lineStart = 0;

        for (usize i = 0; i < _source.len(); i++) {
            if (currentLine == lineNum) {
                lineStart = i;
                break;
            }
            if (_source[i] == '\n') {
                currentLine++;
            }
        }

        if (currentLine != lineNum)
            return ""s;

        usize lineEnd = lineStart;
        while (lineEnd < _source.len() and _source[lineEnd] != '\n')
            lineEnd++;

        return Str{_source.buf() + lineStart, lineEnd - lineStart};
    }

    usize _digitCount(usize n) const {
        if (n == 0)
            return 1;
        usize count = 0;
        while (n > 0) {
            count++;
            n /= 10;
        }
        return count;
    }

    void render(Io::TextWriter& writer, Diagnostic const& diag) const {
        Io::Emit e{writer};

        auto styleReset = Tty::RESET;

        // Header: error[E0001]: message
        e("{}{}", levelStyle(diag.level), levelName(diag.level));
        if (diag.code.len() > 0) {
            e("[{}]", diag.code);
        }
        e("{}: {}{}\n", Tty::RESET, TTY_BOLD, diag.message);
        e("{}", styleReset);

        if (diag.labels.len() == 0) {
            return;
        }

        // Find the maximum line number for padding
        usize maxLine = 0;
        for (auto const& label : diag.labels) {
            maxLine = max(maxLine, label.span.start.line);
            maxLine = max(maxLine, label.span.end.line);
        }
        usize lineNumWidth = _digitCount(maxLine);

        // Location header
        auto const& firstLabel = diag.labels[0];
        e(" {}", TTY_ACCENT);
        _writeSpaces(e, lineNumWidth);
        e("--> {}", styleReset);
        if (not _filename.is<None>()) {
            e("{}:", _filename);
        }
        e("{}\n", firstLabel.span.start);

        // Empty line separator
        e(" {}", TTY_ACCENT);
        _writeSpaces(e, lineNumWidth);
        e(" |{}", styleReset);
        e('\n');

        // Sort labels by line number for proper rendering
        Vec<Label const*> sortedLabels;
        for (auto const& label : diag.labels) {
            sortedLabels.pushBack(&label);
        }

        // Render each unique line with its labels
        Vec<usize> renderedLines;
        for (auto const* label : sortedLabels) {
            usize line = label->span.start.line;

            bool alreadyRendered = false;
            for (auto renderedLine : renderedLines) {
                if (renderedLine == line) {
                    alreadyRendered = true;
                    break;
                }
            }
            if (alreadyRendered)
                continue;
            renderedLines.pushBack(line);

            // Line number and source
            Str lineContent = _lineContent(line);
            e(" {}", TTY_ACCENT);
            // Right-justify line number
            usize digits = _digitCount(line);
            for (usize i = digits; i < lineNumWidth; i++)
                e(' ');
            e("{} |{} {}\n", line, styleReset, lineContent);

            // Underline and label
            e(" {}", TTY_ACCENT);
            _writeSpaces(e, lineNumWidth);
            e(" |{} ", styleReset);

            // Collect all labels for this line
            Vec<Label const*> lineLabels;
            for (auto const* l : sortedLabels) {
                if (l->span.start.line == line) {
                    lineLabels.pushBack(l);
                }
            }

            // Render underlines
            usize lastCol = 1; // Columns are 1-based
            for (auto const* l : lineLabels) {
                usize startCol = l->span.start.col;
                usize endCol = l->span.end.col;

                // Handle same-position spans (single character)
                if (endCol <= startCol) {
                    endCol = startCol + 1;
                }

                // Add padding to reach startCol
                while (lastCol < startCol) {
                    e(' ');
                    lastCol++;
                }

                // Underline character
                auto underStyle = l->isPrimary ? levelStyle(diag.level) : TTY_ACCENT;
                Rune underChar = l->isPrimary ? '^' : '-';

                e("{}", underStyle);
                for (usize i = startCol; i < endCol; i++) {
                    e("{:c}", underChar);
                    lastCol++;
                }

                // Label message on the same line for primary
                if (l->isPrimary and l->message.len() > 0) {
                    e(" {}", l->message);
                }
                e("{}", styleReset);
            }
            e('\n');

            // Render secondary labels on separate lines
            for (auto const* l : lineLabels) {
                if (not l->isPrimary and l->message.len() > 0) {
                    e(" {}", TTY_ACCENT);
                    _writeSpaces(e, lineNumWidth);
                    e(" |{}", styleReset);

                    usize startCol = l->span.start.col;
                    for (usize i = 0; i < startCol; i++) {
                        e(' ');
                    }
                    e("{}{}{}\n", TTY_ACCENT, l->message, styleReset);
                }
            }
        }

        // Empty line separator
        e(" {}", TTY_ACCENT);
        _writeSpaces(e, lineNumWidth);
        e(" |{}", styleReset);
        e('\n');

        // Notes
        for (auto const& note : diag.notes) {
            e(" {}", TTY_ACCENT);
            _writeSpaces(e, lineNumWidth);
            e(" = {}note{}: {}{}\n", levelStyle(Level::NOTE),
              styleReset, note, styleReset);
        }

        // Help
        if (auto const& [help] = diag.help) {
            e(" {}", TTY_ACCENT);
            _writeSpaces(e, lineNumWidth);
            e(" = {}help{}: {}{}\n", levelStyle(Level::HELP),
              styleReset, help, styleReset);
        }

        e('\n');
        (void)e.flush();
    }

    void render(Io::TextWriter& writer, Collector const& collector) {
        for (auto const& diag : collector.diags)
            render(writer, diag);
    }
};

export struct SimpleRenderer {
    Union<None, String, Ref::Url> _filename;

    SimpleRenderer(Union<None, String, Ref::Url> filename = NONE)
        : _filename(filename) {}

    void render(Io::TextWriter& writer, Diagnostic const& diag) const {
        Io::Emit e{writer};

        e("{}{}", levelStyle(diag.level), levelName(diag.level));
        if (diag.code.len() > 0) {
            e("[{}]", diag.code);
        }
        e("{}: {}{}\n", Tty::RESET, TTY_BOLD, diag.message);
        e("{}", Tty::RESET);

        if (diag.labels.len() > 0) {
            auto const& firstLabel = diag.labels[0];
            e(" {}--> {}", TTY_ACCENT, Tty::RESET);

            if (not _filename.is<None>()) {
                e("{}:", _filename);
            }
            e("{}\n", firstLabel.span.start);
        }

        for (auto const& note : diag.notes) {
            e(" {}={}", TTY_ACCENT, Tty::RESET);
            e(" {}note{}: {}{}\n", levelStyle(Level::NOTE),
              Tty::RESET, note, Tty::RESET);
        }

        if (auto const& [help] = diag.help) {
            e(" {}={}", TTY_ACCENT, Tty::RESET);
            e(" {}help{}: {}{}\n", levelStyle(Level::HELP),
              Tty::RESET, help, Tty::RESET);
        }

        e('\n');
        (void)e.flush();
    }

    void render(Io::TextWriter& writer, Collector const& collector) {
        for (auto const& diag : collector.diags)
            render(writer, diag);
    }
};

} // namespace Karm::Diag
