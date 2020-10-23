/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_ROUTINGTABLEIMPL
#define LIBP2P_PROTOCOL_KADEMLIA_ROUTINGTABLEIMPL

#include <deque>

#include <boost/assert.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>
#include <libp2p/protocol/kademlia/routing_table.hpp>

namespace libp2p::protocol::kademlia {

  /**
   * @class Bucket
   *
   * Single bucket which holds peers.
   *
   * @see
   * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/bucket.go
   */
  class Bucket : public std::deque<peer::PeerId> {
   public:
    void truncate(size_t limit) {
      if (size() > limit) {
        erase(std::next(begin(), limit), end());
      }
    }

    std::vector<peer::PeerId> toVector() const {
      return {begin(), end()};
    }

    bool contains(const peer::PeerId &p) const {
      auto it = std::find(begin(), end(), p);
      return it != end();
    }

    bool remove(const peer::PeerId &p) {
      // this shifts elements to the end
      auto it = std::remove(begin(), end(), p);
      if (it != end()) {
        erase(it);
        return true;
      }

      return false;
    }

    void moveToFront(const peer::PeerId &p) {
      remove(p);
	    push_front(p);
    }

    Bucket split(size_t commonLenPrefix, const NodeId &target) {
      Bucket b{};
      // remove shifts all elements "to be removed" to the end, other elements
      // preserve their relative order
      auto new_end =
          std::remove_if(begin(), end(), [&](const peer::PeerId &pid) {
            NodeId nodeId(pid);
            return nodeId.commonPrefixLen(target) > commonLenPrefix;
          });

      b.assign(std::make_move_iterator(new_end),
               std::make_move_iterator(end()));

      return b;
    }
  };

  class RoutingTableImpl : public RoutingTable {
   public:
    ~RoutingTableImpl() override = default;

    enum class Error {
      SUCCESS = 0,
      PEER_REJECTED_HIGH_LATENCY = 1,
      PEER_REJECTED_NO_CAPACITY
    };

    RoutingTableImpl(std::shared_ptr<peer::IdentityManager> identity_manager,
                     std::shared_ptr<event::Bus> bus, const Config &config);

    outcome::result<void> update(const peer::PeerId &pid) override;

    void remove(const peer::PeerId &peer_id) override;

    std::vector<peer::PeerId> getAllPeers() const override;

    std::vector<peer::PeerId> getNearestPeers(const NodeId &node_id,
                                              size_t count) override;

    size_t size() const override;

   private:
    std::vector<Bucket> buckets_;

    std::shared_ptr<peer::IdentityManager> identity_manager_;
    const NodeId local_;
    std::shared_ptr<event::Bus> bus_;
    size_t bucket_size_;

    void nextBucket();

    libp2p::common::Logger log_ = libp2p::common::createLogger("kad");
  };

}  // namespace libp2p::protocol::kademlia

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol::kademlia, RoutingTableImpl::Error);

#endif  // LIBP2P_PROTOCOL_KADEMLIA_ROUTINGTABLEIMPL
