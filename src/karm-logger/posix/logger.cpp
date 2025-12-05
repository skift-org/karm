module;

#include <karm-core/macros.h>
#include <stdio.h>

module Karm.Logger;

namespace Karm::Logger::_Embed {

void loggerLock() {}

void loggerUnlock() {}

Io::TextWriter& loggerOut() {
    struct LoggerOut : Io::TextWriter {
        Res<> writeRune(Rune rune) override {
            Utf8::One one;
            if (not Utf8::encodeUnit(rune, one))
                return Error::invalidInput("encoding error");
            if (fwrite(one.buf(), 1, one.len(), stderr) < one.len()) {
                return Error::other("could not write to stderr");
            }
            return Ok();
        }
    };

    static LoggerOut out;
    return out;
}

} // namespace Karm::Logger::_Embed
