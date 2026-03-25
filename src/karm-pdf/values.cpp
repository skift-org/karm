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

    void hash(Hasher& h) const {
        Karm::hash(h, num);
        Karm::hash(h, gen);
    }
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
    Vec<u8> data;

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
        usize offset = 0;
        usize gen = 0;
        bool used = false;
    };

    Vec<Entry> entries;

    void add(usize num, usize offset, usize gen) {
        if (num >= entries.len()) {
            entries.resize(num + 1);
        }

        entries[num] = {offset, gen, true};
    }

    Res<> write(Io::Writer& writer) const;
};

} // namespace Karm::Pdf
