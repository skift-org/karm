module;

#include <compare>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

export module Karm.Base.Std;

export namespace std {

// <compare>
using std::partial_ordering;
using std::strong_ordering;
using std::weak_ordering;

// <coroutine>
using std::coroutine_handle;
using std::coroutine_traits;
using std::suspend_always;
using std::suspend_never;

// <cstddef>
using std::size_t;
using std::nullptr_t;

// <cstdint>
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;

using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;

// <memory>
using std::construct_at;
using std::destroy_at;
using std::addressof;

// <cstring>
using std::memcpy;
using std::strlen;

// <initializer_list>
using std::initializer_list;

// <type_traits>
using std::integral_constant;

// <utility>
using std::forward;
using std::move;
using std::swap;
using std::exchange;
using std::declval;
using std::integer_sequence;
using std::make_integer_sequence;
using std::index_sequence;
using std::make_index_sequence;

} // namespace std
