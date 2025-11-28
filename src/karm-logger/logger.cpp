export module Karm.Logger;

import Karm.Core;
import Karm.Tty;

export import :_embed;

namespace Karm {

struct Level {
    isize value;
    char const* name;
    Tty::Style style;
};

struct Format {
    Str str;
    Loc loc;

    Format(char const* str, Loc loc = Loc::current())
        : str(str), loc(loc) {
    }

    Format(Str str, Loc loc = Loc::current())
        : str(str), loc(loc) {
    }
};

export constexpr Level PRINT = {-2, "print", Tty::BLUE};
export constexpr Level YAP = {-1, "yappin'", Tty::GREEN};
export constexpr Level DEBUG = {0, "debug", Tty::BLUE};
export constexpr Level INFO = {1, "info ", Tty::GREEN};
export constexpr Level WARNING = {2, "warn ", Tty::YELLOW};
export constexpr Level ERROR = {3, "error", Tty::RED};
export constexpr Level FATAL = {4, "fatal", Tty::style(Tty::RED).bold()};

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

    if (level.value != -2) {
        _catch(Io::format(out, "{} ", level.name | level.style));
        _catch(Io::format(out, "{}{}:{}: ", Tty::reset().fg(Tty::GRAY_DARK), fmt.loc.file, fmt.loc.line));
    }

    _catch(Io::format(out, "{}", Tty::reset()));
    _catch(Io::_format(out, fmt.str, args));
    _catch(Io::format(out, "{}\n", Tty::reset()));

    Logger::_Embed::loggerUnlock();
}

export template <typename... Args>
void logPrint(Format fmt, Args&&... va) {
    Io::Args<Args...> args{std::forward<Args>(va)...};
    _log(PRINT, fmt, args);
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
