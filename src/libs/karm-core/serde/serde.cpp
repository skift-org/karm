module;

#include <karm-core/macros.h>

export module Karm.Core:serde.serde;

import :base.base;
import :base.res;
import :base.string;
import :base.vec;
import :base.symbol;
import :base.map;

namespace Karm::Serde {

export enum struct SizeHint {
    N8,
    N16,
    N32,
    N64,

    AUTO
};

export template <typename T>
    requires Meta::Integral<T> or Meta::Float<T>
SizeHint sizeHintFor() {
    if constexpr (sizeof(T) == 1)
        return SizeHint::N8;
    else if constexpr (sizeof(T) == 2)
        return SizeHint::N16;
    else if constexpr (sizeof(T) == 4)
        return SizeHint::N32;
    else if constexpr (sizeof(T) == 8)
        return SizeHint::N64;
    else
        static_assert(false, "unable to infer size hint");
}

// union
// enum
// alt

// tuple

// array
// vec

// map
// object
// field

export struct Type {
    enum struct Kind {
        NONE,
        SOME,
        UNION,
        FIELD,
        ENUM,
        ARRAY,
        VEC,
        MAP,
    };

    using enum Kind;

    Kind kind;
    usize index;
    usize len;
    Opt<Symbol> tag;
};

template <typename T>
struct Serde;

export struct Serializer;
export struct Deserializer;

export template <typename T>
Res<> serialize(Serializer& ser, T const& v) {
    if constexpr (requires { v.serialize(ser); }) {
        return v.serialize(ser);
    } else if constexpr (requires { Serde<T>::serialize(ser, v); }) {
        return Serde<T>::serialize(ser, v);
    } else {
        static_assert(false, "No Serde<T> found and T::serialize(Serializer&) is not available");
    }
}

export template <typename T>
Res<T> deserialize(Deserializer& de) {
    if constexpr (requires { T::deserialize(de); }) {
        return T::deserialize(de);
    } else if constexpr (requires { Serde<T>::deserialize(de); }) {
        return Serde<T>::deserialize(de);
    } else {
        static_assert(false, "No Serde<T> found and T::deserialize(Deserialize&) is not available");
    }
}

export struct Serializer {
    virtual ~Serializer() = default;

    virtual Res<> serializeBool(bool v) = 0;
    virtual Res<> serializeUnsigned(u64 v, SizeHint hint = SizeHint::AUTO) = 0;
    virtual Res<> serializeInteger(i64 v, SizeHint hint = SizeHint::AUTO) = 0;
    virtual Res<> serializeFloat(f64 v, SizeHint hint = SizeHint::AUTO) = 0;
    virtual Res<> serializeString(Str v) = 0;
    virtual Res<> serializeBytes(Bytes v) = 0;

    virtual Res<> beginUnit(Type) = 0;
    virtual Res<> endUnit() = 0;

    struct Scope {
        Res<> end();
        Serializer* operator->();
    };

    virtual Res<Scope> beginScope(Type);

    Res<> serializeUnit(Type);

    template <typename T>
    Res<> serializeUnit(Type, T);

    template <typename T>
    Res<> serialize(T const&);
};

export struct Deserializer {
    virtual ~Deserializer() = default;

    virtual Res<bool> deserializeBool() = 0;
    virtual Res<u64> deserializeUnsigned(SizeHint hint = SizeHint::AUTO) = 0;
    virtual Res<i64> deserializeInteger(SizeHint hint = SizeHint::AUTO) = 0;
    virtual Res<f64> deserializeFloat(SizeHint hint = SizeHint::AUTO) = 0;
    virtual Res<String> deserializeString() = 0;
    virtual Res<Vec<u8>> deserializeBytes() = 0;

    virtual Res<urange> beginUnit(Type);
    virtual Res<> endUnit();

    struct Scope {
        urange _range;
        Res<> end();
        Deserializer* operator->();
    };

    virtual Res<Scope> beginScope(Type);

    Res<> deserializeUnit(Type);

    template <typename T>
    Res<T> deserializeUnit(Type);

    template <typename T>
    Res<T> deserialize();
};

template <>
struct Serde<bool> {
    static Res<> serialize(Serializer& ser, bool const& v) {
        return ser.serializeBool(v);
    }

    static Res<bool> deserialize(Deserializer& de) {
        return de.deserializeBool();
    }
};

template <>
struct Serde<None> {
    static Res<> serialize(Serializer& ser, None const&) {
        return ser.serializeUnit({Type::NONE});
    }

    static Res<None> deserialize(Deserializer& de) {
        try$(de.deserializeUnit({Type::NONE}));
        return Ok(NONE);
    }
};

template <typename T>
struct Serde<Opt<T>> {
    static Res<> serialize(Serializer& ser, Opt<T> const& v) {
        if (not v)
            return ser.serialize(NONE);
        return ser.serializeUnit({Type::SOME}, v);
    }

    static Res<Opt<T>> deserialize(Deserializer& de) {
        if (auto res = de.deserializeUnit<T>({Type::SOME}))
            return std::move(res);
        return de.deserialize<None>();
    }
};

template <typename V, typename E>
struct Serde<Res<V, E>> {
    static Res<> serialize(Serializer& ser, Res<V, E> const& v) {
        if (not v)
            return ser.serializeUnit<E>({Type::NONE}, v.none());
        return ser.serializeUnit<V>({Type::SOME}, v.unwrap());
    }

    static Res<Res<V, E>> deserialize(Deserializer& de) {
        if (auto res = de.deserializeUnit<V>({Type::SOME}))
            return std::move(res);
        return de.deserializeUnit<E>({Type::NONE});
    }
};

template <typename... Ts>
struct Serde<Union<Ts...>> {
    static Res<> serialize(Serializer& ser, Union<Ts...> const& v) {
        auto scope = try$(ser.beginScope({Type::UNION}));
        try$(v.visit([&](auto const& v) {
            return scope->serializeUnit({Type::FIELD, .index = v.index()}, v);
        }));
        return scope.end();
    }

    static Res<Union<Ts...>> deserialize(Deserializer& de) {
        auto scope = try$(de.beginScope({Type::UNION}));
        usize index = 0;
        auto res = try$(Meta::any<Ts...>([&]<typename T> -> Res<Union<Ts...>> {
            return scope->deserializeUnit<T>({Type::FIELD, .index = index++});
        }));
        try$(scope.end());
        return Ok(std::move(res));
    }
};

template <Meta::Enum T>
struct Serde<T> {
    static Res<> serialize(Serializer& ser, T const& v) {
        auto scope = try$(ser.beginScope({Type::ENUM}));
        try$(scope->serializeUnit({Type::FIELD, .tag = Symbol::from(nameOf(v))}, toUnderlyingType(v)));
        return scope.end();
    }

    static Res<T> deserialize(Deserializer& de) {
        auto scope = try$(de.beginScope({Type::ENUM}));
        for (auto i : enumItems<T>()) {
            if (auto res = de.deserializeUnit<Meta::UnderlyingType<T>>({Type::FIELD, .tag = Symbol::from(i.name)})) {
                try$(scope.end());
                return res;
            }
        }
        return Error::invalidData("expected enum value");
    }
};

template <Meta::Unsigned T>
struct Serde<T> {
    static Res<> serialize(Serializer& ser, T const& v) {
        return ser.serializeUnsigned(v, sizeHintFor<T>());
    }

    static Res<T> deserialize(Deserializer& de) {
        return de.deserializeUnsigned(sizeHintFor<T>());
    }
};

template <Meta::Signed T>
struct Serde<T> {
    static Res<> serialize(Serializer& ser, T const& v) {
        return ser.serializeInteger(v, sizeHintFor<T>());
    }

    static Res<T> deserialize(Deserializer& de) {
        return de.deserializeInteger(sizeHintFor<T>());
    }
};

template <Meta::Float T>
struct Serde<T> {
    static Res<> serialize(Serializer& ser, T const& v) {
        return ser.serializeFloat(v, sizeHintFor<T>());
    }

    static Res<T> deserialize(Deserializer& de) {
        return de.deserializeFloat(sizeHintFor<T>());
    }
};

template <>
struct Serde<String> {
    static Res<> serialize(Serializer& ser, String const& v) {
        return ser.serializeString(v);
    }

    static Res<String> deserialize(Deserializer& de) {
        return de.deserializeString();
    }
};

template <>
struct Serde<Vec<u8>> {
    static Res<> serialize(Serializer& ser, Vec<u8> const& v) {
        return ser.serializeBytes(v);
    }

    static Res<Vec<T>> deserialize(Deserializer& de) {
        return de.deserializeBytes();
    }
};

template <typename T, usize N>
struct Serde<Array<T, N>> {
    static Res<> serialize(Serializer& ser, Array<T, N> const& v) {
        auto scope = try$(ser.beginScope({Type::ARRAY}));
        for (auto& i : v) {
            try$(scope->serialize(i));
        }
        return scope.end();
    }

    static Res<Array<T, N>> deserialize(Deserializer& de) {
        auto scope = try$(de.beginScope({Type::ARRAY}));
        Array<T, N> res;
        for (auto& i : res) {
            i = try$(scope->deserialize<T>(i));
        }
        return Ok(std::move(res));
    }
};

template <typename T>
struct Serde<Vec<T>> {
    static Res<> serialize(Serializer& ser, Vec<T> const& v) {
        auto scope = try$(ser.beginScope({.kind = Type::VEC, .len = v.len()}));
        for (auto& i : v) {
            try$(scope->serialize(i));
        }
        return scope.end();
    }

    static Res<Vec<T>> deserialize(Deserializer& de) {
        auto scope = try$(de.beginScope({Type::VEC}));
        Vec<T> res;
        while (not scope.ended()) {
            res.pushBack(try$(scope->deserialize<T>()));
        }
        try$(scope.end());
        return Ok(std::move(res));
    }
};

template <typename T>
struct Serde<Map<String, T>> {
    static Res<> serialize(Serializer& ser, Map<String, T> const& v) {
        auto scope = try$(ser.beginScope({.kind = Type::MAP, .len = v.len()}));
        for (auto& [k, v] : v.iter())
            try$(scope->serializeUnit({.kind = Type::FIELD, .tag = Symbol::from(k)}, v));
        return scope.end();
    }

    static Res<Map<String, T>> deserialize(Deserializer& de);
};

template <typename... Ts>
struct Serde<Tuple<Ts...>> {
    static Res<> serialize(Serializer& ser, Tuple<Ts...> const& v);

    static Res<Tuple<Ts...>> deserialize(Deserializer& de);
};

template <Meta::Aggregate T>
struct Serde<T> {
    static Res<> serialize(Serializer& ser, T const& v);

    static Res<T> deserialize(Deserializer& de);
};

} // namespace Karm::Serde