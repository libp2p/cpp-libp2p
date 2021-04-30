/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <mutex>
#include <string_view>
#include <unordered_map>

#define LIBP2P_INSTANCES(type) ::libp2p::Instances libp2p_instances{#type}

namespace libp2p {
  struct Instances {
    static std::mutex mutex;
    static std::unordered_map<std::string_view, std::atomic_size_t> counts;

    decltype(counts)::mapped_type *count;

    inline Instances(std::string_view key) {
      std::unique_lock lock{mutex};
      count = &counts[key];
      lock.unlock();
      ++*count;
    }
    inline ~Instances() {
      --*count;
    }
  };
}  // namespace libp2p
