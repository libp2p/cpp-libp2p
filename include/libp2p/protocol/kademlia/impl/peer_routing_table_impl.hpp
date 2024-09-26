/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/protocol/kademlia/impl/peer_routing_table.hpp>

#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <list>

#include <libp2p/event/bus.hpp>
#include <libp2p/log/sublogger.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>

namespace libp2p::protocol::kademlia {
  constexpr size_t kBucketCount = 256;

  struct BucketPeerInfo {
    peer::PeerId peer_id;
    bool is_replaceable;
    bool is_connected;
    NodeId node_id;
    BucketPeerInfo(const PeerId &peer_id,
                   bool is_replaceable,
                   bool is_connected)
        : peer_id{peer_id},
          is_replaceable{is_replaceable},
          is_connected{is_connected},
          node_id{peer_id} {}
  };

  struct XorDistanceComparator {
    bool operator()(const BucketPeerInfo &a, const BucketPeerInfo &b) {
      auto d1 = a.node_id.distance(from);
      auto d2 = b.node_id.distance(from);
      constexpr auto size = Hash256().size();

      // return true, if distance d1 is less than d2, false otherwise
      return std::memcmp(d1.data(), d2.data(), size) < 0;
    }

    NodeId from;
  };

  /**
   * Single bucket which holds peers.
   */
  class Bucket {
   public:
    size_t size() const;

    void append(const Bucket &bucket);

    // sort bucket in ascending order by XOR distance from node_id
    void sort(const NodeId &node_id);

    auto find(const peer::PeerId &p) const;

    bool moveToFront(const PeerId &pid);

    void emplaceToFront(const PeerId &pid,
                        bool is_replaceable,
                        bool is_connected);

    boost::optional<PeerId> removeReplaceableItem();

    void truncate(size_t limit);

    std::vector<peer::PeerId> peerIds() const;

    bool contains(const peer::PeerId &p) const;

    bool remove(const peer::PeerId &p);

   private:
    std::list<BucketPeerInfo> peers_;
  };

  class PeerRoutingTableImpl
      : public PeerRoutingTable,
        public std::enable_shared_from_this<PeerRoutingTableImpl> {
   public:
    ~PeerRoutingTableImpl() override = default;

    enum class Error {
      SUCCESS = 0,
      PEER_REJECTED_HIGH_LATENCY = 1,
      PEER_REJECTED_NO_CAPACITY
    };

    PeerRoutingTableImpl(
        const Config &config,
        std::shared_ptr<peer::IdentityManager> identity_manager,
        std::shared_ptr<event::Bus> bus);

    outcome::result<bool> update(const peer::PeerId &pid,
                                 bool is_permanent,
                                 bool is_connected = false) override;

    void remove(const peer::PeerId &peer_id) override;

    std::vector<peer::PeerId> getAllPeers() const override;

    std::vector<peer::PeerId> getNearestPeers(const NodeId &node_id,
                                              size_t count) override;

    size_t size() const override;

   private:
    std::optional<size_t> getBucketIndex(const NodeId &key) const;

    const Config &config_;
    std::shared_ptr<peer::IdentityManager> identity_manager_;
    std::shared_ptr<event::Bus> bus_;

    const NodeId local_;

    std::array<Bucket, kBucketCount> buckets_;
  };

}  // namespace libp2p::protocol::kademlia

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia,
                          PeerRoutingTableImpl::Error);
