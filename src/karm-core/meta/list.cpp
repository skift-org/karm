module;

#include <karm/macros>

export module Karm.Core:meta.list;

import :meta.traits;
import :meta.pack;

namespace Karm::Meta {

export template <typename...>
struct List {};

// for testing purposes
template <typename...>
struct _AnotherList {};

// MARK: List Rename -----------------------------------------------------------

template <typename From, template <typename...> typename To>
struct _ListRename;

template <template <typename...> typename From, typename... Ts, template <typename...> typename To>
struct _ListRename<From<Ts...>, To> {
    using Type = To<Ts...>;
};

export template <typename From, template <typename...> typename To>
using ListRename = _ListRename<From, To>::Type;

static_assert(Same<ListRename<List<int, float>, List>, List<int, float>>);
static_assert(Same<ListRename<List<int, float>, _AnotherList>, _AnotherList<int, float>>);

// MARK: List Flatten ----------------------------------------------------------

template <template <typename...> typename L, typename Acc, typename... Ts>
struct _ListFlatten {
    using Type = Acc;
};

template <template <typename...> typename L, typename... Ts, typename... Us, typename... Vs>
struct _ListFlatten<L, List<Ts...>, L<Us...>, Vs...> {
    using Type = _ListFlatten<L, List<Ts...>, Us..., Vs...>::Type;
};

template <template <typename...> typename L, typename... Ts, typename A, typename... Us>
struct _ListFlatten<L, List<Ts...>, A, Us...> {
    using Type = _ListFlatten<L, List<Ts..., A>, Us...>::Type;
};

template <typename U>
struct _ListFlattenOf;

template <template <typename...> typename L, typename... Ts>
struct _ListFlattenOf<L<Ts...>> {
    using Type = ListRename<typename _ListFlatten<L, List<>, Ts...>::Type, L>;
};

export template <typename U>
using ListFlatten = _ListFlattenOf<U>::Type;

static_assert(Same<ListFlatten<List<int, List<float>, List<>, List<char>>>, List<int, float, char>>);
static_assert(Same<ListFlatten<List<int, List<float>, List<>, List<char, List<>>>>, List<int, float, char>>);
static_assert(Same<ListFlatten<List<int, _AnotherList<float>, List<>, _AnotherList<char>>>, List<int, _AnotherList<float>, _AnotherList<char>>>);

// MARK: List Uniq -------------------------------------------------------------

template <typename Acc, typename... Ts>
struct _ListUniq {
    using Type = Acc;
};

template <typename... Ts, Contains<Ts...> A, typename... Us>
struct _ListUniq<List<Ts...>, A, Us...> {
    using Type = _ListUniq<List<Ts...>, Us...>::Type;
};

template <typename... Ts, typename A, typename... Us>
struct _ListUniq<List<Ts...>, A, Us...> {
    using Type = _ListUniq<List<Ts..., A>, Us...>::Type;
};

template <typename U>
struct _ListUniqOf;

template <template <typename...> typename L, typename... Ts>
struct _ListUniqOf<L<Ts...>> {
    using Type = ListRename<typename _ListUniq<List<>, Ts...>::Type, L>;
};

export template <typename U>
using ListUniq = _ListUniqOf<U>::Type;

static_assert(Same<ListUniq<List<int, float, int>>, List<int, float>>);
static_assert(Same<ListUniq<List<int, float, int, float>>, List<int, float>>);
static_assert(Same<ListUniq<List<int, _AnotherList<float>>>, List<int, _AnotherList<float>>>);

// MARK: List Decay ------------------------------------------------------------

template <typename U>
struct _ListDecay {
    using Type = U;
};

template <template <typename...> typename L, typename T>
struct _ListDecay<L<T>> {
    using Type = T;
};

export template <typename U>
using ListDecay = _ListDecay<U>::Type;

static_assert(Same<ListDecay<List<int>>, int>);
static_assert(Same<ListDecay<List<int, float>>, List<int, float>>);

} // namespace Karm::Meta
