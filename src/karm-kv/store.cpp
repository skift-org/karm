module;

#include <karm/macros>

export module Karm.Kv:store;

import Karm.Crypto;
import Karm.Core;
import Karm.Logger;

import :blob;
import :wal;

namespace Karm::Kv {

export struct Store {
    Rc<Wal> _wal;
    Map<Blob, Blob> _memdb;

    static Rc<Store> open(Rc<Wal> wal) {
        auto db = makeRc<Store>(wal);
        for (Wal::Record const& r : wal->iter()) {
            if (r.type == Wal::PUT) {
                db->_memdb.put(r.key, r.value);
            } else if (r.type == Wal::DEL) {
                if (db->_memdb.remove(r.key))
                    logWarn("could not replay del record because of missing key");
            }
        }
        return db;
    };

    Res<> put(Bytes key, Bytes value) {
        try$(_wal->record(Wal::PUT, key, value));
        _memdb.put(Blob::from(key), Blob::from(value));
        return Ok();
    }

    Res<> del(Bytes key) {
        try$(_wal->record(Wal::DEL, key, {}));
        try$(_memdb.remove(Blob::from(key)).okOr(Error::notFound()));
        return Ok();
    }
};

} // namespace Karm::Kv
