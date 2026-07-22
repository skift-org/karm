export module Karm.Logger;

import Karm.Core;
import Karm.Tty;
import Karm.Debug;

export import :_embed;

namespace Karm {

static constexpr Tty::Style TTY_LOCATION = {.foreground = Tty::GRAY_DARK, .bold = true};

struct Level {
    isize value;
    char const* name;
    Tty::Style style;
};

struct Format {
    Str str;
    SourceLocation location;

    Format(char const* str, SourceLocation location = SourceLocation::current())
        : str(str), location(location) {
    }

    Format(Str str, SourceLocation location = SourceLocation::current())
        : str(str), location(location) {
    }
};

export constexpr Level YAP = {.value = -1, .name = "yappin'", .style = {.foreground = Tty::GREEN}};
export constexpr Level DEBUG = {.value = 0, .name = "debug", .style = {.foreground = Tty::BLUE}};
export constexpr Level INFO = {.value = 1, .name = "info", .style = {.foreground = Tty::GREEN}};
export constexpr Level WARNING = {.value = 2, .name = "warn", .style = {.foreground = Tty::YELLOW}};
export constexpr Level ERROR = {.value = 3, .name = "error", .style = {.foreground = Tty::RED}};
export constexpr Level FATAL = {.value = 4, .name = "fatal", .style = {.foreground = Tty::RED, .bold = true}};

static auto debugLogLocation = Debug::Flag::debug("logger-location", "Show log statement filename and line number.");

void _catch(Res<> res) {
    if (res)
        return;
    debug("failed to write to logger");
    panic(res.none().msg());
}

isize _logLevel = -2;

export void setLogLevel(Level level) {
    _logLevel = level.value;
}

void _log(Level level, Format fmt, Io::_Args& args) {
    if (level.value < _logLevel)
        return;

    Logger::_Embed::loggerLock();
    auto& out = Logger::_Embed::loggerOut();

    _catch(Io::format(out, "{}: ", level.name | level.style));
    if (debugLogLocation)
        _catch(Io::format(out, "{}: ", fmt.location | TTY_LOCATION));
    _catch(Io::_format(out, fmt.str, args));
    _catch(Io::format(out, "{}\n", Tty::RESET));

    Logger::_Embed::loggerUnlock();
}

export template <typename... Args>
void logPrintIf(bool condition, Format fmt, Args&&... va) {
    if (condition)
        logPrint(fmt, std::forward<Args>(va)...);
}

export template <typename... Args>
void logDebug(Format fmt, Args&&... va) {
    Io::Args<Args...> args{std::forward<Args>(va)...};
    _log(DEBUG, fmt, args);
}

export template <typename... Args>
void logDebugIf(bool condition, Format fmt, Args&&... va) {
    if (condition)
        logDebug(fmt, std::forward<Args>(va)...);
}

export template <typename... Args>
void logInfo(Format fmt, Args&&... va) {
    Io::Args<Args...> args{std::forward<Args>(va)...};
    _log(INFO, fmt, args);
}

export template <typename... Args>
void logInfoIf(bool condition, Format fmt, Args&&... va) {
    if (condition)
        logInfo(fmt, std::forward<Args>(va)...);
}

export template <typename... Args>
void yap(Format fmt, Args&&... va) {
    Io::Args<Args...> args{std::forward<Args>(va)...};
    _log(YAP, fmt, args);
}

export template <typename... Args>
void logWarn(Format fmt, Args&&... va) {
    Io::Args<Args...> args{std::forward<Args>(va)...};
    _log(WARNING, fmt, args);
}

export template <typename... Args>
void logWarnIf(bool condition, Format fmt, Args&&... va) {
    if (condition)
        logWarn(fmt, std::forward<Args>(va)...);
}

export template <typename... Args>
void logError(Format fmt, Args&&... va) {
    Io::Args<Args...> args{std::forward<Args>(va)...};
    _log(ERROR, fmt, args);
}

export template <typename... Args>
void logErrorIf(bool condition, Format fmt, Args&&... va) {
    if (condition)
        logError(fmt, std::forward<Args>(va)...);
}

export template <typename... Args>
[[noreturn]] void logFatal(Format fmt, Args&&... va) {
    Io::Args<Args...> args{std::forward<Args>(va)...};
    _log(FATAL, fmt, args);
    panic("fatal error occurred, see logs");
}

} // namespace Karm
