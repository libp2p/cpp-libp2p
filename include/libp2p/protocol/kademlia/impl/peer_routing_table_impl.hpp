/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_PEERROUTINGTABLEIMPL
#define LIBP2P_PROTOCOL_KADEMLIA_PEERROUTINGTABLEIMPL

#include <libp2p/protocol/kademlia/impl/peer_routing_table.hpp>

#include <boost/assert.hpp>
#include <deque>

#include <libp2p/event/bus.hpp>
#include <libp2p/log/sublogger.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>

namespace libp2p::protocol::kademlia {

  struct BucketPeerInfo {
    peer::PeerId peer_id;
    bool is_replaceable;
    NodeId node_id;
    BucketPeerInfo(const peer::PeerId &peer_id, bool is_replaceable)
        : peer_id(peer_id), is_replaceable(is_replaceable), node_id(peer_id) {}
  };

  struct XorDistanceComparator {
    explicit XorDistanceComparator(const peer::PeerId &from) {
      crypto::Sha256 hash;
      hash.write(from.toVector()).value();
      memcpy(hfrom.data(), hash.digest().value().data(),
             std::min<size_t>(hash.digestSize(), hfrom.size()));
    }

    explicit XorDistanceComparator(const NodeId &from)
        : hfrom(from.getData()) {}

    explicit XorDistanceComparator(const Hash256 &hash) : hfrom(hash) {}

    bool operator()(const BucketPeerInfo &a, const BucketPeerInfo &b) {
      NodeId from(hfrom);
      auto d1 = a.node_id.distance(from);
      auto d2 = b.node_id.distance(from);
      constexpr auto size = Hash256().size();

      // return true, if distance d1 is less than d2, false otherwise
      return std::memcmp(d1.data(), d2.data(), size) < 0;
    }

    Hash256 hfrom;
  };
  /**
   * Single bucket which holds peers.
   */
  class Bucket : public std::deque<BucketPeerInfo> {
   public:
    void truncate(size_t limit) {
      if (size() > limit) {
        erase(std::next(begin(), limit), end());
      }
    }

    std::vector<peer::PeerId> peerIds() const {
      std::vector<peer::PeerId> peerIds;
      peerIds.reserve(size());
      std::transform(begin(), end(), std::back_inserter(peerIds),
                     [](const auto &bpi) { return bpi.peer_id; });
      return peerIds;
    }

    bool contains(const peer::PeerId &p) {
      auto it = std::find_if(begin(), end(),
                             [=](const auto &bpi) { return bpi.peer_id == p; });
      return it != end();
    }

    bool remove(const peer::PeerId &p) {
      // this shifts elements to the end
      auto it = std::remove_if(
          begin(), end(), [&](const auto &bpi) { return bpi.peer_id == p; });
      if (it != end()) {
        erase(it);
        return true;
      }

      return false;
    }

    Bucket split(size_t commonLenPrefix, const NodeId &target) {
      Bucket b{};
      // remove shifts all elements "to be removed" to the end, other elements
      // preserve their relative order
      auto new_end = std::remove_if(begin(), end(), [&](const auto &bpi) {
        return bpi.node_id.commonPrefixLen(target) > commonLenPrefix;
      });

      b.assign(std::make_move_iterator(new_end),
               std::make_move_iterator(end()));

      return b;
    }
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

    outcome::result<bool> update(const peer::PeerId &pid, bool is_permanent,
                                 bool is_connected=false) override;

    void remove(const peer::PeerId &peer_id) override;

    std::vector<peer::PeerId> getAllPeers() const override;

    std::vector<peer::PeerId> getNearestPeers(const NodeId &node_id,
                                              size_t count) override;

    size_t size() const override;

   private:
    void nextBucket();

    const Config &config_;
    std::shared_ptr<peer::IdentityManager> identity_manager_;
    std::shared_ptr<event::Bus> bus_;

    const NodeId local_;

    std::vector<Bucket> buckets_;

    log::SubLogger log_;
  };

}  // namespace libp2p::protocol::kademlia

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia,
                          PeerRoutingTableImpl::Error);

#endif  // LIBP2P_PROTOCOL_KADEMLIA_PEERROUTINGTABLEIMPL
