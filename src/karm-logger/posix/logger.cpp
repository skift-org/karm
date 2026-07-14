module;

#include <karm/macros>
#include <stdio.h>

module Karm.Logger;

namespace Karm::Logger::_Embed {

void loggerLock() {}

void loggerUnlock() {}

Io::TextWriter& loggerOut() {
    struct LoggerOut : Io::TextWriter {
        Io::BufferWriter _bw;

        Res<> flush() {
            auto bytes = _bw.bytes();
            if (bytes.len() > 0) {
                if (fwrite(bytes.buf(), 1, bytes.len(), stderr) < bytes.len())
                    return Error::other("could not write to stderr");
                _bw.clear();
            }
            return Ok();
        }

        Res<> writeRune(Rune rune) override {
            Utf8::One one;
            if (not Utf8::encodeUnit(rune, one))
                return Error::invalidInput("encoding error");

            try$(_bw.write(bytes(one)));

            if (rune == '\n')
                try$(flush());

            return Ok();
        }
    };

    static LoggerOut out;
    return out;
}

} // namespace Karm::Logger::_Embed
