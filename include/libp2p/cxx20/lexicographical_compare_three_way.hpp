/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __APPLE__
#include <compare>
#include <type_traits>
#else
#include <algorithm>
#endif

namespace libp2p::cxx20 {
#ifdef __APPLE__
  /// https://en.cppreference.com/w/cpp/algorithm/lexicographical_compare_three_way
  template <class I1, class I2, class Cmp>
  constexpr auto lexicographical_compare_three_way(
      I1 f1, I1 l1, I2 f2, I2 l2, Cmp comp) -> decltype(comp(*f1, *f2)) {
    using ret_t = decltype(comp(*f1, *f2));
    static_assert(
        std::disjunction_v<std::is_same<ret_t, std::strong_ordering>,
                           std::is_same<ret_t, std::weak_ordering>,
                           std::is_same<ret_t, std::partial_ordering>>,
        "The return type must be a comparison category type.");

    bool exhaust1 = (f1 == l1);
    bool exhaust2 = (f2 == l2);
    for (; !exhaust1 && !exhaust2;
         exhaust1 = (++f1 == l1), exhaust2 = (++f2 == l2)) {
      if (auto c = comp(*f1, *f2); c != 0) {
        return c;
      }
    }

    return !exhaust1 ? std::strong_ordering::greater
         : !exhaust2 ? std::strong_ordering::less
                     : std::strong_ordering::equal;
  }

#if !defined(__cpp_lib_three_way_comparison)
  // primarily fix for AppleClang < 15

  // clang-format off
  // https://github.com/llvm/llvm-project/blob/main/libcxx/include/__compare/compare_three_way.h
  struct _LIBCPP_TEMPLATE_VIS compare_three_way
  {
    template<class _T1, class _T2>
    constexpr _LIBCPP_HIDE_FROM_ABI
    auto operator()(_T1&& __t, _T2&& __u) const
    noexcept(noexcept(_VSTD::forward<_T1>(__t) <=> _VSTD::forward<_T2>(__u)))
    { return          _VSTD::forward<_T1>(__t) <=> _VSTD::forward<_T2>(__u); }

    using is_transparent = void;
  };
  // clang-format on
#else
  using std::compare_three_way;
#endif

  template <class I1, class I2>
  constexpr auto lexicographical_compare_three_way(I1 f1, I1 l1, I2 f2, I2 l2) {
    return ::libp2p::cxx20::lexicographical_compare_three_way(
        f1, l1, f2, l2, compare_three_way{});
  }
#else
  using std::lexicographical_compare_three_way;
#endif
}  // namespace libp2p::cxx20
