/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <deque>
#include <libp2p/peer/peer_id.hpp>
#include <qtils/bytes_std_hash.hpp>
#include <qtils/empty.hpp>
#include <unordered_map>
#include <unordered_set>

namespace libp2p::protocol::gossip {
  using MessageId = Bytes;
}  // namespace libp2p::protocol::gossip

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

    size_t size() const {
      return map_.size();
    }

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

    void popFront() {
      if (expirations_.empty()) {
        throw std::logic_error{"TimeCache::popFront empty"};
      }
      map_.erase(expirations_.front().second);
      expirations_.pop_front();
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

  template <typename K>
  class IDontWantCache {
    static constexpr size_t kCapacity = 10000;
    static constexpr Ttl kTtl = std::chrono::seconds{3};

   public:
    void clearExpired(Time now = Clock::now()) {
      cache_.clearExpired(now);
    }

    bool contains(const K &key) const {
      return cache_.contains(key);
    }

    void insert(const K &key) {
      if (cache_.contains(key)) {
        return;
      }
      if (cache_.size() >= kCapacity) {
        cache_.popFront();
      }
      cache_.getOrDefault(key);
    }

   private:
    TimeCache<K, qtils::Empty> cache_{kTtl};
  };

  class GossipPromises {
   public:
    GossipPromises(Ttl ttl) : ttl_{ttl} {}

    bool contains(const MessageId &message_id) const {
      return map_.contains(message_id);
    }

    void add(const MessageId &message_id,
             const PeerId &peer_id,
             Time now = Clock::now()) {
      map_[message_id].emplace(peer_id, now + ttl_);
    }

    void remove(const MessageId &message_id) {
      map_.erase(message_id);
    }

    void peers(const MessageId &message_id, const auto &f) {
      auto it = map_.find(message_id);
      if (it != map_.end()) {
        for (auto &p : it->second) {
          f(p.first);
        }
      }
    }

    auto clearExpired(Time now = Clock::now()) {
      std::unordered_map<PeerId, size_t> result;
      for (auto it1 = map_.begin(); it1 != map_.end();) {
        auto &map2 = it1->second;
        for (auto it2 = map2.begin(); it2 != map2.end();) {
          if (it2->second < now) {
            ++result[it2->first];
            it2 = map2.erase(it2);
          } else {
            ++it2;
          }
        }
        if (map2.empty()) {
          it1 = map_.erase(it1);
        } else {
          ++it1;
        }
      }
      return result;
    }

   private:
    Ttl ttl_;
    std::unordered_map<MessageId,
                       std::unordered_map<PeerId, Time>,
                       qtils::BytesStdHash>
        map_;
  };
}  // namespace libp2p::protocol::gossip::time_cache

namespace libp2p::protocol::gossip {
  using time_cache::DuplicateCache;
  using time_cache::GossipPromises;
  using time_cache::IDontWantCache;
  using time_cache::TimeCache;
}  // namespace libp2p::protocol::gossip
