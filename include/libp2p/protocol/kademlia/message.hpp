/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_MESSAGE
#define LIBP2P_PROTOCOL_KADEMLIA_MESSAGE

#include <libp2p/network/connection_manager.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  /// Wire protocol message. May be either request or response
  struct Message {
    enum class Type {
      kPutValue = 0,
      kGetValue = 1,
      kAddProvider = 2,
      kGetProviders = 3,
      kFindNode = 4,
      kPing = 5,

      kTableSize
    };

    struct Record {
      Key key;
      Value value;
      std::string time_received;
    };

    using Connectedness = network::ConnectionManager::Connectedness;

    struct Peer {
      PeerInfo info;
      Connectedness conn_status = Connectedness::NOT_CONNECTED;
    };
    using Peers = std::vector<Peer>;

    Type type = Type::kPing;
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
    void selfAnnounce(PeerInfo self);
  };

  Message createPutValueRequest(const Key &key, const Value &value);

  Message createGetValueRequest(const Key &key,
                                boost::optional<PeerInfo> self_announce);

  Message createAddProviderRequest(PeerInfo self, const Key &key);

  Message createGetProvidersRequest(const Key &key,
                                    boost::optional<PeerInfo> self_announce);

  Message createFindNodeRequest(const PeerId &node,
                                boost::optional<PeerInfo> self_announce);

}  // namespace libp2p::protocol::kademlia
#endif  // LIBP2P_PROTOCOL_KADEMLIA_MESSAGE
