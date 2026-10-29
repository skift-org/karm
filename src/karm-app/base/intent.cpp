export module Karm.App.Base:intent;

import Karm.Core;
import Karm.Ref;

namespace Karm::App {

export struct Attachment {
    Ref::Url url;
    Opt<Ref::Uti> type = NONE;

    void repr(Io::Emit& e) const {
        e("(object url:{} type:{})", url, type);
    }
};

export struct Intent {
    Ref::Uti action = Ref::Uti::PUBLIC_OPEN;
    Vec<Attachment> attachments = {};
    Opt<Ref::Url> handler = NONE;
    Opt<Ref::Url> callback = NONE;

    void repr(Io::Emit& e) const {
        e("(intent action:{} attachments:{} handler:{} callback:{})", action, attachments, handler, callback);
    }
};

} // namespace Karm::App
