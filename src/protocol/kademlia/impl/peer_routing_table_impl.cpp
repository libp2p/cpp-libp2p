/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/peer_routing_table_impl.hpp>

#include <numeric>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol::kademlia,
                            PeerRoutingTableImpl::Error,
                            e) {
  using E = libp2p::protocol::kademlia::PeerRoutingTableImpl::Error;

  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::PEER_REJECTED_NO_CAPACITY:
      return "peer has been rejected - no capacity";
    case E::PEER_REJECTED_HIGH_LATENCY:
      return "peer has been rejected - high latency";
    default:
      break;
  }

  return "unknown error";
}

namespace libp2p::protocol::kademlia {

  size_t Bucket::size() const {
    return peers_.size();
  }

  void Bucket::append(const Bucket &bucket) {
    peers_.insert(peers_.end(), bucket.peers_.begin(), bucket.peers_.end());
  }

  void Bucket::sort(const NodeId &node_id) {
    XorDistanceComparator cmp{node_id};
    peers_.sort(cmp);
  }

  auto findPeer(auto &peers, const peer::PeerId &p) {
    return std::ranges::find_if(peers,
                                [&p](const auto &i) { return i.peer_id == p; });
  }

  auto Bucket::find(const peer::PeerId &p) const {
    return findPeer(peers_, p);
  }

  bool Bucket::moveToFront(const PeerId &pid) {
    auto it = findPeer(peers_, pid);
    if (it != peers_.end()) {
      it->is_connected = true;
      if (it != peers_.begin()) {
        peers_.splice(peers_.begin(), peers_, it);
      }
      return false;
    }
    return true;
  }

  void Bucket::emplaceToFront(const PeerId &pid,
                              bool is_replaceable,
                              bool is_connected) {
    peers_.emplace(peers_.begin(), pid, is_replaceable, is_connected);
  }

  boost::optional<PeerId> Bucket::removeReplaceableItem() {
    boost::optional<PeerId> result;

    for (auto it = peers_.rbegin(); it != peers_.rend(); ++it) {
      // https://github.com/libp2p/rust-libp2p/blob/3837e33cd4c40ae703138e6aed6f6c9d52928a80/protocols/kad/src/kbucket/bucket.rs#L310-L366
      if (it->is_replaceable and not it->is_connected) {
        result = std::move(it->peer_id);
        peers_.erase((++it).base());
        break;
      }
    }

    return result;
  }

  void Bucket::truncate(size_t limit) {
    if (limit == 0) {
      peers_.clear();
    } else if (peers_.size() > limit) {
      peers_.erase(std::next(peers_.begin(), static_cast<long>(limit)),
                   peers_.end());
    }
  }

  std::vector<peer::PeerId> Bucket::peerIds() const {
    std::vector<peer::PeerId> peerIds;
    peerIds.reserve(peers_.size());
    std::transform(peers_.begin(),
                   peers_.end(),
                   std::back_inserter(peerIds),
                   [](const auto &bpi) { return bpi.peer_id; });
    return peerIds;
  }

  bool Bucket::contains(const peer::PeerId &p) const {
    return find(p) != peers_.end();
  }

  bool Bucket::remove(const peer::PeerId &p) {
    auto it = find(p);
    if (it != peers_.end()) {
      peers_.erase(it);
      return true;
    }

    return false;
  }

  PeerRoutingTableImpl::PeerRoutingTableImpl(
      const Config &config,
      std::shared_ptr<peer::IdentityManager> identity_manager,
      std::shared_ptr<event::Bus> bus)
      : config_(config),
        identity_manager_(std::move(identity_manager)),
        bus_(std::move(bus)),
        local_([this] {
          BOOST_ASSERT(identity_manager_ != nullptr);
          return identity_manager_->getId();
        }()) {
    BOOST_ASSERT(identity_manager_ != nullptr);
    BOOST_ASSERT(bus_ != nullptr);
    BOOST_ASSERT(config_.maxBucketSize > 1);
  }

  void PeerRoutingTableImpl::remove(const peer::PeerId &peer_id) {
    auto bucket_index = getBucketIndex(NodeId{peer_id});
    if (not bucket_index) {
      return;
    }
    auto &bucket = buckets_.at(*bucket_index);
    bucket.remove(peer_id);
    bus_->getChannel<event::protocol::kademlia::PeerRemovedChannel>().publish(
        peer_id);
  }

  // https://github.com/libp2p/rust-libp2p/blob/3837e33cd4c40ae703138e6aed6f6c9d52928a80/protocols/kad/src/kbucket.rs#L400-L461
  std::vector<peer::PeerId> PeerRoutingTableImpl::getNearestPeers(
      const NodeId &node_id, size_t count) {
    auto distance = local_.distance(node_id);
    // check if distance bit is set
    // https://github.com/libp2p/rust-libp2p/blob/3837e33cd4c40ae703138e6aed6f6c9d52928a80/protocols/kad/src/kbucket.rs#L409-L428
    auto bit = [&](size_t i) {
      auto j = 255 - i;
      return ((distance[j / 8] >> (7 - j % 8)) & 1) != 0;
    };
    auto bucket_index = getBucketIndex(node_id);
    Bucket result;
    auto done = [&] { return result.size() >= count; };
    auto append = [&](size_t i) { result.append(buckets_.at(i)); };
    if (bucket_index) {
      if (auto i = *bucket_index) {
        append(i);
        for (--i; i != 0 and not done(); --i) {
          if (bit(i)) {
            append(i);
          }
        }
      }
    }
    if (not done()) {
      append(0);
    }
    for (size_t i = 1; i < 256 and not done(); ++i) {
      if (not bit(i)) {
        append(i);
      }
    }
    result.sort(node_id);
    result.truncate(count);
    return result.peerIds();
  }

  namespace {
    outcome::result<bool> replacePeer(Bucket &bucket,
                                      const peer::PeerId &pid,
                                      bool is_replaceable,
                                      bool is_connected,
                                      event::Bus &bus) {
      const auto removed = bucket.removeReplaceableItem();
      if (!removed.has_value()) {
        return PeerRoutingTableImpl::Error::PEER_REJECTED_NO_CAPACITY;
      }
      bus.getChannel<event::protocol::kademlia::PeerRemovedChannel>().publish(
          removed.value());
      bucket.emplaceToFront(pid, is_replaceable, is_connected);
      bus.getChannel<event::protocol::kademlia::PeerAddedChannel>().publish(
          pid);
      return true;
    }
  }  // namespace

  outcome::result<bool> PeerRoutingTableImpl::update(const peer::PeerId &pid,
                                                     bool is_permanent,
                                                     bool is_connected) {
    auto bucket_index = getBucketIndex(NodeId{pid});
    if (not bucket_index) {
      return true;
    }
    auto &bucket = buckets_.at(*bucket_index);

    // Trying to find and move to front if its a long lived connected peer
    if (is_connected) {
      if (!bucket.moveToFront(pid)) {
        return false;
      }
    } else if (bucket.contains(pid)) {
      return false;
    }

    if (bucket.size() < config_.maxBucketSize) {
      bucket.emplaceToFront(pid, not is_permanent, is_connected);
      bus_->getChannel<event::protocol::kademlia::PeerAddedChannel>().publish(
          pid);
      return true;
    }

    return replacePeer(bucket, pid, not is_permanent, is_connected, *bus_);
  }

  size_t PeerRoutingTableImpl::size() const {
    return std::accumulate(
        buckets_.begin(),
        buckets_.end(),
        0u,
        [](size_t num, const Bucket &bucket) { return num + bucket.size(); });
  }

  std::vector<peer::PeerId> PeerRoutingTableImpl::getAllPeers() const {
    std::vector<peer::PeerId> vec;
    for (const auto &bucket : buckets_) {
      auto peer_ids = bucket.peerIds();
      vec.insert(vec.end(), peer_ids.begin(), peer_ids.end());
    }
    return vec;
  }

  // https://github.com/libp2p/rust-libp2p/blob/3837e33cd4c40ae703138e6aed6f6c9d52928a80/protocols/kad/src/kbucket.rs#L141-L150
  std::optional<size_t> PeerRoutingTableImpl::getBucketIndex(
      const NodeId &key) const {
    if (local_ == key) {
      return std::nullopt;
    }
    return 255 - local_.commonPrefixLen(key);
  }
}  // namespace libp2p::protocol::kademlia
