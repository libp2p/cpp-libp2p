/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_METRICS_INSTANCE_LIST_HPP
#define LIBP2P_COMMON_METRICS_INSTANCE_LIST_HPP

#include <list>
#include <mutex>

#define _LIBP2P_METRICS_INSTANCE_LIST(type)                          \
  struct Libp2pMetricsInstanceList {                                 \
    using List = std::list<const type *>;                            \
    struct State {                                                   \
      std::mutex mutex;                                              \
      List list;                                                     \
      static auto &get() {                                           \
        static State state;                                          \
        return state;                                                \
      }                                                              \
    };                                                               \
    List::const_iterator it;                                         \
    Libp2pMetricsInstanceList(const type *ptr) {                     \
      auto &state{State::get()};                                     \
      std::lock_guard lock{state.mutex};                             \
      state.list.emplace_front(ptr);                                 \
      it = state.list.begin();                                       \
    }                                                                \
    Libp2pMetricsInstanceList(const Libp2pMetricsInstanceList &list) \
        : Libp2pMetricsInstanceList{                                 \
            (const type *)((uintptr_t)*list.it + (this - &list))} {} \
    ~Libp2pMetricsInstanceList() {                                   \
      auto &state{State::get()};                                     \
      std::lock_guard lock{state.mutex};                             \
      state.list.erase(it);                                          \
    }                                                                \
    void operator=(const Libp2pMetricsInstanceList &hook) {}         \
  } libp2p_metrics_instance_list {                                   \
    this                                                             \
  }

#define LIBP2P_METRICS_INSTANCE_LIST(type) \
  _LIBP2P_METRICS_INSTANCE_LIST(::type)  // require qualified name

#endif  // LIBP2P_COMMON_METRICS_INSTANCE_LIST_HPP
