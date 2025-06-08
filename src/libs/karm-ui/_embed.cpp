module;

#include <karm-base/rc.h>
#include <karm-sys/context.h>

export module Karm.Ui:_embed;

namespace Karm::Ui {

struct Node;

} // namespace Karm::Ui

namespace Karm::Ui::_Embed {

Async::Task<> runAsync(Sys::Context&, Rc<Node> root);

} // namespace Karm::Ui::_Embed
