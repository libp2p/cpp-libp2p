/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/peer_id.hpp>

namespace libp2p::protocol::gossip {
  using TopicId = std::string;
  using MessageId = Bytes;

  class Score {
   public:
    bool below(const PeerId &peer_id, double threshold) {
      return false;
    }
    void addPenalty(const PeerId &peer_id, size_t count) {}
    void graft(const PeerId &peer_id, const TopicId &topic) {}
    void prune(const PeerId &peer_id, const TopicId &topic) {}
    void duplicateMessage(const PeerId &peer_id,
                          const MessageId &msg_id,
                          const TopicId &topic) {}
    void validateMessage(const PeerId &peer_id,
                         const MessageId &msg_id,
                         const TopicId &topic) {}
    void connect(const PeerId &peer_id) {}
    void disconnect(const PeerId &peer_id) {}

    void onDecay() {}
  };
}  // namespace libp2p::protocol::gossip
