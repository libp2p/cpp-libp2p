/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

namespace libp2p::protocol::gossip {
  using TopicId = std::string;

  constexpr double kDefaultDecayToZero = 0.1;

  struct TopicScoreParams {
    double topic_weight = 0.5;
    double time_in_mesh_weight = 1;
    std::chrono::milliseconds time_in_mesh_quantum{1};
    double time_in_mesh_cap = 3600;
    double first_message_deliveries_weight = 1;
    double first_message_deliveries_decay = 0.5;
    double first_message_deliveries_cap = 2000;
    double mesh_message_deliveries_weight = -1;
    double mesh_message_deliveries_decay = 0.5;
    double mesh_message_deliveries_cap = 100;
    double mesh_message_deliveries_threshold = 20;
    std::chrono::milliseconds mesh_message_deliveries_window{10};
    std::chrono::seconds mesh_message_deliveries_activation{5};
    double mesh_failure_penalty_weight = -1;
    double mesh_failure_penalty_decay = 0.5;
    double invalid_message_deliveries_weight = -1;
    double invalid_message_deliveries_decay = 0.3;
  };

  struct ScoreConfig {
    double zero = 0;
    /// The score threshold below which gossip propagation is suppressed;
    /// should be negative.
    double gossip_threshold = -10;
    /// The score threshold below which we shouldn't publish when using flood
    /// publishing (also applies to fanout peers); should be negative and <=
    /// `gossip_threshold`.
    double publish_threshold = -50;
    /// The score threshold below which message processing is suppressed
    /// altogether, implementing an effective graylist according to peer score;
    /// should be negative and
    /// <= `publish_threshold`.
    double graylist_threshold = -80;
    /// The median mesh score threshold before triggering opportunistic
    /// grafting; this should have a small positive value.
    double opportunistic_graft_threshold = 20;

    std::unordered_map<TopicId, TopicScoreParams> topics;
    double topic_score_cap = 3600;
    double app_specific_weight = 10;
    double behaviour_penalty_weight = -10;
    double behaviour_penalty_threshold = 0;
    double behaviour_penalty_decay = 0.2;
    std::chrono::milliseconds decay_interval = std::chrono::seconds{1};
    double decay_to_zero = kDefaultDecayToZero;
    std::chrono::seconds retain_score{3600};
    double slow_peer_weight = -0.2;
    double slow_peer_threshold = 0.0;
    double slow_peer_decay = 0.2;

    bool valid() const {
      if (gossip_threshold > 0) {
        return false;
      }
      if (publish_threshold > 0 or publish_threshold > gossip_threshold) {
        return false;
      }
      if (graylist_threshold > 0 or graylist_threshold > publish_threshold) {
        return false;
      }
      if (opportunistic_graft_threshold < 0) {
        return false;
      }
      return true;
    }
  };
}  // namespace libp2p::protocol::gossip
