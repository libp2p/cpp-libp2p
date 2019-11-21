/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_MESSAGE_HPP
#define LIBP2P_KAD_MESSAGE_HPP

#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/network/connection_manager.hpp>

namespace libp2p::protocol::kademlia {

  /// Message from wire protocol. Maybe either request or response
  struct Message {
    enum Type {
      kPutValue = 0,
      kGetValue = 1,
      kAddProvider = 2,
      kGetProviders = 3,
      kFindNode = 4,
      kPing = 5
    };

    struct Record {
      Key key;
      Value value;
      std::string time_received;
    };

    struct Peer {
      peer::PeerInfo info;
      network::ConnectionManager::Connectedness conn_status
        = network::ConnectionManager::Connectedness::NOT_CONNECTED;
    };
    using Peers = std::vector<Peer>;

    Type type = kPing;
    Key key;
    std::optional<Record> record;
    std::optional<Peers> closer_peers;
    std::optional<Peers> provider_peers;

    void clear();
    bool deserialize(const void* data, size_t sz);
    bool serialize(std::vector<uint8_t>& buffer) const;
  };

  using MessageCallback = std::function<void(outcome::result<Message>)>;
}
#endif //LIBP2P_KAD_MESSAGE_HPP
