/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <random>

#include "peer_context.hpp"

namespace libp2p::protocol::gossip {
  class ChoosePeers {
   public:
    std::deque<PeerContextPtr> choose(
        auto &&all,
        const std::invocable<PeerContextPtr> auto &predicate,
        const std::invocable<size_t> auto &get_count) {
      std::deque<PeerContextPtr> chosen;
      for (auto &ctx : all) {
        if (ctx->isGossipsub() and predicate(ctx)) {
          chosen.emplace_back(ctx);
        }
      }
      std::ranges::shuffle(chosen, random_);
      auto count = get_count(chosen.size());
      if (chosen.size() > count) {
        chosen.resize(count);
      }
      return chosen;
    }
    std::deque<PeerContextPtr> choose(
        auto &&all,
        const std::invocable<PeerContextPtr> auto &predicate,
        size_t count) {
      return choose(std::forward<decltype(all)>(all), predicate, [&](size_t) {
        return count;
      });
    }

   private:
    std::default_random_engine random_;
  };
}  // namespace libp2p::protocol::gossip
