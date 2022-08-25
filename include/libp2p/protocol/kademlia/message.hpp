/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_MESSAGE
#define LIBP2P_PROTOCOL_KADEMLIA_MESSAGE

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/kademlia/common.hpp>

namespace libp2p::protocol::kademlia {

  /// Wire protocol message. May be either request or response
  struct Message {
    enum class Error {
      INVALID_CONNECTEDNESS = 1,
      INVALID_PEER_ID,
      INVALID_ADDRESSES,
    };

    enum class Type {
      kPutValue = 0,
      kGetValue = 1,
      kAddProvider = 2,
      kGetProviders = 3,
      kFindNode = 4,
      kPing = 5,
    };

    struct Record {
      Key key;
      Value value;
      std::string time_received;
    };

    using Connectedness = Host::Connectedness;

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

    const std::string &errorMessage() const {
      return error_message_;
    }

   private:
    std::string error_message_;
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

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia, Message::Error);

#endif  // LIBP2P_PROTOCOL_KADEMLIA_MESSAGE
