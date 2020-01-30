/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_MESSAGE_HPP
#define LIBP2P_KAD_MESSAGE_HPP

#include <libp2p/network/connection_manager.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  /// Wire protocol message. May be either request or response
  struct Message {
    enum Type {
      kPutValue = 0,
      kGetValue = 1,
      kAddProvider = 2,
      kGetProviders = 3,
      kFindNode = 4,
      kPing = 5,

      kTableSize
    };

    struct Record {
      ContentAddress key;
      Value value;
      std::string time_received;
    };

    using Connectedness = network::ConnectionManager::Connectedness;

    struct Peer {
      peer::PeerInfo info;
      Connectedness conn_status = Connectedness::NOT_CONNECTED;
    };
    using Peers = std::vector<Peer>;

    Type type = kPing;
    std::vector<uint8_t> key;
    boost::optional<Record> record;
    boost::optional<Peers> closer_peers;
    boost::optional<Peers> provider_peers;

    void clear();

    // tries to deserialize message from byte array
    bool deserialize(const void *data, size_t sz);

    // serializes varint(message length) + message into buffer
    bool serialize(std::vector<uint8_t> &buffer) const;

    // adds this peer listening address to closer_peers
    void selfAnnounce(peer::PeerInfo self);
  };

  // self is a protocol extension if this is server (i.e. announce)
  Message createFindNodeRequest(const peer::PeerId &node,
                                boost::optional<peer::PeerInfo> self_announce);

  Message createPutValueRequest(const ContentAddress &key, Value value);

  Message createGetValueRequest(const ContentAddress &key);

  Message createAddProviderRequest(peer::PeerInfo self,
                                   const ContentAddress &key);

}  // namespace libp2p::protocol::kademlia
#endif  // LIBP2P_KAD_MESSAGE_HPP
