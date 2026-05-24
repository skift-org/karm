export module Karm.Ipc:serde;

import Karm.Core;
import Karm.Sys;

namespace Karm::Ipc {

export struct MessageSerializer : Serde::PackSerializer {
    Vec<Sys::Handle> _handles = {};
    bool _inHandle = false;

    MessageSerializer(Io::BEmit& emit)
        : PackSerializer(emit) {
    }

    void give(Sys::Handle hnd) {
        _handles.emplaceBack(hnd);
    }

    Slice<Sys::Handle> handles() const {
        return _handles;
    }

    void clear() {
        _handles.clear();
    }

    Res<> beginUnit(Serde::Type t) override {
        if (t.kind == Serde::Type::UNIT and t.tag == "__handle__")
            _inHandle = true;
        return PackSerializer::beginUnit(t);
    }

    Res<> endUnit() override {
        _inHandle = false;
        return PackSerializer::endUnit();
    }

    Res<> serializeUnsigned(u64 v, Serde::SizeHint hint) override {
        if (_inHandle) {
            give(Sys::Handle{v});
            return Ok();
        }
        return PackSerializer::serializeUnsigned(v, hint);
    }
};

export struct MessageDeserializer : Serde::PackDeserializer {
    Cursor<Sys::Handle> _handles;
    bool _inHandle = false;

    MessageDeserializer(Io::BScan& scan, Slice<Sys::Handle> handles)
        : PackDeserializer(scan), _handles(handles) {
    }

    Sys::Handle take() {
        if (_handles.ended())
            return Sys::INVALID_HANDLE;
        return _handles.next();
    }

    Res<Serde::Type> beginUnit(Serde::Type t) override {
        if (t.kind == Serde::Type::UNIT and t.tag == "__handle__")
            _inHandle = true;
        return PackDeserializer::beginUnit(t);
    }

    Res<> endUnit() override {
        _inHandle = false;
        return PackDeserializer::endUnit();
    }

    Res<u64> deserializeUnsigned(Serde::SizeHint hint) override {
        if (_inHandle)
            return Ok(take().value());
        return PackDeserializer::deserializeUnsigned(hint);
    }
};

} // namespace Karm::Ipc