/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <deque>
#include <qtils/empty.hpp>
#include <unordered_map>
#include <unordered_set>

namespace libp2p::protocol::gossip::time_cache {
  using Ttl = std::chrono::milliseconds;
  using Clock = std::chrono::steady_clock;
  using Time = Clock::time_point;

  /// This implements a time-based LRU cache for checking gossipsub message
  /// duplicates.
  template <typename K, typename V>
  class TimeCache {
   public:
    TimeCache(Ttl ttl) : ttl_{ttl} {}

    bool contains(const K &key) const {
      return map_.contains(key);
    }

    void clearExpired(Time now = Clock::now()) {
      while (not expirations_.empty() and expirations_.front().first <= now) {
        map_.erase(expirations_.front().second);
        expirations_.pop_front();
      }
    }

    V &getOrDefault(const K &key, Time now = Clock::now()) {
      clearExpired(now);
      auto it = map_.find(key);
      if (it == map_.end()) {
        it = map_.emplace(key, V()).first;
        expirations_.emplace_back(now + ttl_, it);
      }
      return it->second;
    }

   private:
    using Map = std::unordered_map<K, V>;

    Ttl ttl_;
    Map map_;
    std::deque<std::pair<Clock::time_point, typename Map::iterator>>
        expirations_;
  };

  template <typename K>
  class DuplicateCache {
   public:
    DuplicateCache(Ttl ttl) : cache_{ttl} {}

    bool contains(const K &key) const {
      return cache_.contains(key);
    }

    bool insert(const K &key, Time now = Clock::now()) {
      cache_.clearExpired(now);
      if (cache_.contains(key)) {
        return false;
      }
      cache_.getOrDefault(key);
      return true;
    }

   private:
    TimeCache<K, qtils::Empty> cache_;
  };
}  // namespace libp2p::protocol::gossip::time_cache

namespace libp2p::protocol::gossip {
  using time_cache::DuplicateCache;
  using time_cache::TimeCache;
}  // namespace libp2p::protocol::gossip
