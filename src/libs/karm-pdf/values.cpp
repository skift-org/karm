export module Karm.Pdf:values;

import Karm.Core;

namespace Karm::Pdf {

export struct Value;

export struct Ref {
    usize num{};
    usize gen{};

    Ref alloc() {
        return {++num, gen};
    }

    u64 hash() const {
        return (u64(num) << 32) | u64(gen);
    }

    bool operator==(Ref const& other) const = default;
    auto operator<=>(Ref const& other) const = default;
};

export struct Name : String {
    using String::String;

    void write(Io::Emit& e) const;
};

export struct Array : Vec<Value> {
    using Vec<Value>::Vec;

    void write(Io::Emit& e) const;
};

export struct Dict : Map<Name, Value> {
    using Map<Name, Value>::Map;

    Dict(std::initializer_list<Pair<Name, Value>>&& list) : Map<Name, Value>(fromList(std::move(list))) {}

    void write(Io::Emit& e) const;
};

export struct Stream {
    Dict dict;
    Buf<u8> data;

    void write(Io::Emit& e) const;
};

export using _Value = Union<
    None,
    Ref,
    bool,
    isize,
    usize,
    f64,
    String,
    Name,
    Array,
    Dict,
    Stream>;

export struct Value : _Value {
    using _Value::_Value;

    void write(Io::Emit& e) const;
};

export struct File {
    String header;
    Vec<Pair<Ref, Value>> body;
    Set<Ref> usedRefs;
    Dict trailer;

    Ref add(Ref ref, Value Value) {
        if(usedRefs.has(ref)) {
            panic("PDF Ref reused");
        }
        usedRefs.put(ref);
        body.pushBack({ref, Value});
        return ref;
    }

    Res<> write(Io::Writer& e) const;
};

export struct XRef {
    struct Entry {
        usize offset;
        usize gen;
        bool used;
    };

    Vec<Entry> entries;

    void add(usize offset, usize gen) {
        entries.pushBack({offset, gen, true});
    }

    void write(Io::Emit& e) const;
};

} // namespace Karm::Pdf
