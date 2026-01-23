module;

#include <karm-core/macros.h>

export module Karm.Diag;

import Karm.Core;
import Karm.Tty;
import Karm.Ref;

using namespace Karm;

namespace Diag {

// MARK: Diagnostic Level ------------------------------------------------------

export enum struct Level {
    ERROR,
    WARNING,
    NOTE,
    HELP,

    _LEN,
};

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

    static Tty::Style _levelStyle(Level level) {
        switch (level) {
        case Level::ERROR:
            return Tty::style(Tty::RED).bold();
        case Level::WARNING:
            return Tty::style(Tty::YELLOW).bold();
        case Level::NOTE:
            return Tty::style(Tty::CYAN).bold();
        case Level::HELP:
            return Tty::style(Tty::GREEN).bold();
        default:
            return Tty::reset();
        }
    }

    static Str _levelName(Level level) {
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

        auto styleReset = Tty::reset();
        auto styleBold = Tty::Style{}.bold();
        auto styleBlue = Tty::style(Tty::BLUE).bold();

        // Header: error[E0001]: message
        e("{}{}", _levelStyle(diag.level), _levelName(diag.level));
        if (diag.code.len() > 0) {
            e("[{}]", diag.code);
        }
        e("{}: {}{}\n", styleReset, styleBold, diag.message);
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
        e(" {}", styleBlue);
        _writeSpaces(e, lineNumWidth);
        e("--> {}", styleReset);
        if (not _filename.is<None>()) {
            e("{}:", _filename);
        }
        e("{}\n", firstLabel.span.start);

        // Empty line separator
        e(" {}", styleBlue);
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
            e(" {}", styleBlue);
            // Right-justify line number
            usize digits = _digitCount(line);
            for (usize i = digits; i < lineNumWidth; i++)
                e(' ');
            e("{} |{} {}\n", line, styleReset, lineContent);

            // Underline and label
            e(" {}", styleBlue);
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
                auto underStyle = l->isPrimary ? _levelStyle(diag.level) : styleBlue;
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
                    e(" {}", styleBlue);
                    _writeSpaces(e, lineNumWidth);
                    e(" |{}", styleReset);

                    usize startCol = l->span.start.col;
                    for (usize i = 0; i < startCol; i++) {
                        e(' ');
                    }
                    e("{}{}{}\n", styleBlue, l->message, styleReset);
                }
            }
        }

        // Empty line separator
        e(" {}", styleBlue);
        _writeSpaces(e, lineNumWidth);
        e(" |{}", styleReset);
        e('\n');

        // Notes
        for (auto const& note : diag.notes) {
            e(" {}", styleBlue);
            _writeSpaces(e, lineNumWidth);
            e(" = {}note{}: {}{}\n", _levelStyle(Level::NOTE),
              styleReset, note, styleReset);
        }

        // Help
        if (diag.help) {
            e(" {}", styleBlue);
            _writeSpaces(e, lineNumWidth);
            e(" = {}help{}: {}{}\n", _levelStyle(Level::HELP),
              styleReset, diag.help.unwrap(), styleReset);
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

    static Tty::Style _levelStyle(Level level) {
        switch (level) {
        case Level::ERROR:
            return Tty::style(Tty::RED).bold();
        case Level::WARNING:
            return Tty::style(Tty::YELLOW).bold();
        case Level::NOTE:
            return Tty::style(Tty::CYAN).bold();
        case Level::HELP:
            return Tty::style(Tty::GREEN).bold();
        default:
            return Tty::reset();
        }
    }

    static Str _levelName(Level level) {
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

    void render(Io::TextWriter& writer, Diagnostic const& diag) const {
        Io::Emit e{writer};

        auto styleReset = Tty::reset();
        auto styleBold = Tty::Style{}.bold();
        auto styleBlue = Tty::style(Tty::BLUE).bold();

        e("{}{}", _levelStyle(diag.level), _levelName(diag.level));
        if (diag.code.len() > 0) {
            e("[{}]", diag.code);
        }
        e("{}: {}{}\n", styleReset, styleBold, diag.message);
        e("{}", styleReset);

        if (diag.labels.len() > 0) {
            auto const& firstLabel = diag.labels[0];
            e(" {}--> {}", styleBlue, styleReset);

            if (not _filename.is<None>()) {
                e("{}:", _filename);
            }
            e("{}\n", firstLabel.span.start);
        }

        for (auto const& note : diag.notes) {
            e(" {}={}", styleBlue, styleReset);
            e(" {}note{}: {}{}\n", _levelStyle(Level::NOTE),
              styleReset, note, styleReset);
        }

        if (diag.help) {
            e(" {}={}", styleBlue, styleReset);
            e(" {}help{}: {}{}\n", _levelStyle(Level::HELP),
              styleReset, diag.help.unwrap(), styleReset);
        }

        e('\n');
        (void)e.flush();
    }

    void render(Io::TextWriter& writer, Collector const& collector) {
        for (auto const& diag : collector.diags)
            render(writer, diag);
    }
};

} // namespace Diag
