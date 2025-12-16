export module Karm.Ui:app;

import Karm.Sys;

import :host;
import :_embed;

namespace Karm::Ui {

export Async::Task<> runAsync(Sys::Context& ctx, Child root, Async::CancellationToken ct) {
    co_return co_await _Embed::runAsync(ctx, std::move(root), ct);
}

} // namespace Karm::Ui
