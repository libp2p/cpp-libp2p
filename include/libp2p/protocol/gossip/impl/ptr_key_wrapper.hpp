/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PTR_KEY_WRAPPER_HPP
#define LIBP2P_PTR_KEY_WRAPPER_HPP

#include <memory>

namespace libp2p {

  /**
   * Helper to use smart pointers as a key in associative containers
   *
   * e.g.
   * std::multiset<UniquePtrKeyWrapper<SomeType>, std::less<>> s;
   *   s.emplace(std::make_unique<SomeType>(...));
   *   SomeType other(...);
   *   s.find(other);
   */
  template <typename PtrType>
  struct PtrKeyWrapper {
    const PtrType ptr;

    explicit PtrKeyWrapper(PtrType p) : ptr(std::move(p)) {}

    [[nodiscard]] bool empty() const {
      return !ptr;
    }

    bool operator<(const PtrKeyWrapper<PtrType> &other) const {
      if (empty() or other.empty()) {
        return false;
      }
      return *ptr < *other.ptr;
    }

    bool operator<(const PtrType &other) const {
      if (empty() or !other) {
        return false;
      }
      return *ptr < *other;
    }

    friend bool operator<(const PtrType &a, const PtrKeyWrapper<PtrType> &b) {
      if (!a or b.empty()) {
        return false;
      }
      return *a < *b.ptr;
    }

    template <typename K>
    friend bool operator<(const PtrKeyWrapper &x,
                          const K &key) {
      if (x.empty()) {
        return false;
      }
      return *x.ptr < key;
    }

    template <typename K>
    friend bool operator<(const K &key,
                          const PtrKeyWrapper &x) {
      if (x.empty()) {
        return false;
      }
      return key < *x.ptr;
    }

    template <typename K>
    friend bool operator<(const K &key,
                          const PtrType &x) {
      if (x.empty()) {
        return false;
      }
      return key < *x;
    }


    bool operator==(const PtrKeyWrapper<PtrType> &other) const {
      if (empty()) {
        return other.empty();
      }
      if (other.empty()) {
        return false;
      }
      return *ptr == *other.ptr;
    }

    bool operator==(const typename PtrType::element_type &other) const {
      if (empty()) {
        return false;
      }
      return *ptr == other;
    }

    auto operator-> () const {
      return ptr.operator->();
    }

    auto operator*() const {
      return ptr.operator*();
    }
  };

  template <typename T>
  using SharedPtrKeyWrapper = PtrKeyWrapper<std::shared_ptr<T>>;
  template <typename T>
  using UniquePtrKeyWrapper = PtrKeyWrapper<std::unique_ptr<T>>;

}  // namespace libp2p

namespace std {
  template <typename PtrType>
  struct hash<libp2p::PtrKeyWrapper<PtrType>> {
    const std::hash<typename PtrType::element_type> h;
    size_t operator()(const libp2p::PtrKeyWrapper<PtrType> &object) const {
      return h(*object);
    }
  };
}  // namespace std

#endif  // LIBP2P_PTR_KEY_WRAPPER_HPP
