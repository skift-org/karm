export module Karm.Ui:_embed;

import Karm.Sys;
import :node;

namespace Karm::Ui::_Embed {

export Async::Task<> runAsync(Sys::Context&, Child root, Async::CancellationToken ct);

} // namespace Karm::Ui::_Embed
