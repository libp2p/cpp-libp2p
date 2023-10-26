/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common.hpp"

namespace pubsub::pb {
  // protobuf message forward declaration
  class RPC;
}  // namespace pubsub::pb

namespace libp2p::protocol::gossip {

  class MessageReceiver;

  /// Protobuf message parser.
  class MessageParser {
   public:
    MessageParser();

    ~MessageParser();

    /// Parses RPC protobuf message received from wire
    bool parse(BytesIn bytes);

    /// Dispatches parts of parsed aggregate message to receiver
    void dispatch(const PeerContextPtr &from, MessageReceiver &receiver);

   private:
    /// Parsed protobuf message
    std::unique_ptr<pubsub::pb::RPC> pb_msg_;
  };

}  // namespace libp2p::protocol::gossip
