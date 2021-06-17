/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_METRICS_INSTANCE_COUNT_HPP
#define LIBP2P_COMMON_METRICS_INSTANCE_COUNT_HPP

#include <atomic>
#include <mutex>
#include <string_view>
#include <unordered_map>

#define _LIBP2P_METRICS_INSTANCE_COUNT(type, key)                    \
  struct Libp2pMetricsInstanceCount {                                \
    static auto &count() {                                           \
      static auto &count = ([]() -> auto & {                         \
        auto &state{::libp2p::metrics::instance::State::get()};      \
        std::lock_guard lock{state.mutex};                           \
        return state.counts[key];                                    \
      })();                                                          \
      return count;                                                  \
    }                                                                \
    Libp2pMetricsInstanceCount(const type *) {                       \
      ++count();                                                     \
    }                                                                \
    Libp2pMetricsInstanceCount(const Libp2pMetricsInstanceCount &) { \
      ++count();                                                     \
    }                                                                \
    ~Libp2pMetricsInstanceCount() {                                  \
      --count();                                                     \
    }                                                                \
  } libp2p_metrics_instance_count {                                  \
    this                                                             \
  }
#define LIBP2P_METRICS_INSTANCE_COUNT(type) \
  _LIBP2P_METRICS_INSTANCE_COUNT(::type, #type)  // require qualified name

#ifdef LIBP2P_METRICS_ENABLED
#define LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(...) \
  LIBP2P_METRICS_INSTANCE_COUNT(__VA_ARGS__)
#else
#define LIBP2P_METRICS_INSTANCE_COUNT_IF_ENABLED(...)
#endif

namespace libp2p::metrics::instance {
  struct State {
    std::mutex mutex;
    std::unordered_map<std::string_view, std::atomic_size_t> counts;
    static auto &get() {
      static State state;
      return state;
    }
  };
}  // namespace libp2p::metrics::instance

#endif  // LIBP2P_COMMON_METRICS_INSTANCE_COUNT_HPP
