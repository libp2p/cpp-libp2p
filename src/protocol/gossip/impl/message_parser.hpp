/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_MESSAGE_PARSER_HPP
#define LIBP2P_PROTOCOL_GOSSIP_MESSAGE_PARSER_HPP

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
    bool parse(gsl::span<const uint8_t> bytes);

    /// Dispatches parts of parsed aggregate message to receiver
    void dispatch(const PeerContextPtr &from, MessageReceiver &receiver);

   private:
    /// Parsed protobuf message
    std::unique_ptr<pubsub::pb::RPC> pb_msg_;
  };

} //namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_MESSAGE_PARSER_HPP
