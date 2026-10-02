export module Karm.Core:meta.cond;

namespace Karm::Meta {

template <bool cond, typename TTrue, typename TFalse>
struct _Cond {
    using Type = TTrue;
};

template <typename TTrue, typename TFalse>
struct _Cond<false, TTrue, TFalse> {
    using Type = TFalse;
};

export template <bool cond, typename TTrue, typename TFalse>
using Cond = typename _Cond<cond, TTrue, TFalse>::Type;

template <typename T>
struct _Identity {
    using Type = T;
};

export template <typename T>
using Identity = typename _Identity<T>::Type;

} // namespace Karm::Meta
