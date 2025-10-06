export module Karm.Logger:_embed;

import Karm.Core;

namespace Karm::Logger::_Embed {

export void loggerLock();

export void loggerUnlock();

export Io::TextWriter& loggerOut();

} // namespace Karm::Logger::_Embed
