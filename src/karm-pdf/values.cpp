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

    bool operator==(Ref const& other) const = default;
    auto operator<=>(Ref const& other) const = default;
};

export struct Name : String {
    using String::String;

    Res<> write(Io::Writer& writer) const;
};

export struct Array : Vec<Value> {
    using Vec<Value>::Vec;

    Res<> write(Io::Writer& writer) const;
};

export struct Dict : Map<Name, Value> {
    using Map::Map;

    Res<> write(Io::Writer& writer) const;
};

export struct Stream {
    Dict dict;
    Buf<u8> data;

    Res<> write(Io::Writer& writer) const;
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

    Res<> write(Io::Writer& writer) const;
};

export struct File {
    String header;
    Map<Ref, Value> body;
    Dict trailer;

    Ref add(Ref ref, Value value) {
        body.put(ref, std::move(value));
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

    Res<> write(Io::Writer& writer) const;
};

} // namespace Karm::Pdf
