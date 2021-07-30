/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/peer_routing_table_impl.hpp>

#include <numeric>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol::kademlia,
                            PeerRoutingTableImpl::Error, e) {
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

namespace {
  using libp2p::protocol::kademlia::Bucket;

  size_t getBucketId(const std::vector<Bucket> &buckets, size_t cpl) {
    BOOST_ASSERT(not buckets.empty());
    return cpl >= buckets.size() ? buckets.size() - 1 : cpl;
  }
}  // namespace

namespace libp2p::protocol::kademlia {

  size_t Bucket::size() const {
    return peers_.size();
  }

  void Bucket::append(const Bucket &bucket) {
    peers_.insert(peers_.end(), bucket.peers_.begin(), bucket.peers_.end());
  }

  void Bucket::sort(const NodeId &node_id) {
    XorDistanceComparator cmp(node_id);
    peers_.sort(cmp);
  }

  auto Bucket::find(const peer::PeerId &p) const {
    return std::find_if(peers_.begin(), peers_.end(),
                        [&p](const auto &i) { return i.peer_id == p; });
  }

  bool Bucket::moveToFront(const PeerId &pid) {
    auto it = find(pid);
    if (it != peers_.end()) {
      if (it != peers_.begin()) {
        peers_.splice(peers_.begin(), peers_, it);
      }
      return false;
    }
    return true;
  }

  void Bucket::emplaceToFront(const PeerId &pid, bool is_replaceable) {
    peers_.emplace(peers_.begin(), pid, is_replaceable);
  }

  boost::optional<PeerId> Bucket::removeReplaceableItem() {
    boost::optional<PeerId> result;

    for (auto it = peers_.rbegin(); it != peers_.rend(); ++it) {
      if (it->is_replaceable) {
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
    std::transform(peers_.begin(), peers_.end(), std::back_inserter(peerIds),
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

  Bucket Bucket::split(size_t commonLenPrefix, const NodeId &target) {
    Bucket b{};

    std::list<BucketPeerInfo> new_peers;

    while (!peers_.empty()) {
      auto it = peers_.begin();
      if (it->node_id.commonPrefixLen(target) > commonLenPrefix) {
        b.peers_.splice(b.peers_.end(), peers_, it);
      } else {
        new_peers.splice(new_peers.end(), peers_, it);
      }
    }

    peers_.swap(new_peers);

    return b;
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
        }()),
        log_("PeerRoutingTable", "kademlia") {
    BOOST_ASSERT(identity_manager_ != nullptr);
    BOOST_ASSERT(bus_ != nullptr);
    BOOST_ASSERT(config_.maxBucketSize > 1);
    buckets_.emplace_back();  // create initial bucket
  }

  void PeerRoutingTableImpl::remove(const peer::PeerId &peer_id) {
    size_t cpl = NodeId(peer_id).commonPrefixLen(local_);
    auto bucket_id = getBucketId(buckets_, cpl);
    auto &bucket = buckets_.at(bucket_id);
    bucket.remove(peer_id);
    bus_->getChannel<events::PeerRemovedChannel>().publish(peer_id);
  }

  std::vector<peer::PeerId> PeerRoutingTableImpl::getNearestPeers(
      const NodeId &node_id, size_t count) {
    size_t cpl = node_id.commonPrefixLen(local_);
    size_t bucketId = getBucketId(buckets_, cpl);

    // make a copy of a bucket
    auto bucket = buckets_.at(bucketId);
    if (bucket.size() < count) {
      // In the case of an unusual split, one bucket may be short or empty.
      // if this happens, search both surrounding buckets for nearby peers
      if (bucketId > 0) {
        auto &left = buckets_.at(bucketId - 1);
        bucket.append(left);
      }
      if (bucketId < buckets_.size() - 1) {
        auto &right = buckets_.at(bucketId + 1);
        bucket.append(right);
      }
    }

    // sort bucket in ascending order by XOR distance from local peer.
    bucket.sort(node_id);

    bucket.truncate(count);

    return bucket.peerIds();
  }

  namespace {
    outcome::result<bool> replacePeer(Bucket &bucket, const peer::PeerId &pid,
                                      bool is_replaceable, event::Bus &bus) {
      const auto removed = bucket.removeReplaceableItem();
      if (!removed.has_value()) {
        return PeerRoutingTableImpl::Error::PEER_REJECTED_NO_CAPACITY;
      }
      bus.getChannel<events::PeerRemovedChannel>().publish(removed.value());
      bucket.emplaceToFront(pid, is_replaceable);
      bus.getChannel<events::PeerAddedChannel>().publish(pid);
      return true;
    }
  }  // namespace

  outcome::result<bool> PeerRoutingTableImpl::update(const peer::PeerId &pid,
                                                     bool is_permanent,
                                                     bool is_connected) {
    NodeId nodeId(pid);
    size_t cpl = nodeId.commonPrefixLen(local_);

    auto bucketId = getBucketId(buckets_, cpl);
    auto &bucket = buckets_.at(bucketId);

    // Trying to find and move to front if its a long lived connected peer
    if (is_connected) {
      if (!bucket.moveToFront(pid)) {
        return false;
      }
    } else if (bucket.contains(pid)) {
      return false;
    }

    if (bucket.size() < config_.maxBucketSize) {
      bucket.emplaceToFront(pid, !is_permanent);
      bus_->getChannel<events::PeerAddedChannel>().publish(pid);
      return true;
    }

    if (bucketId == buckets_.size() - 1) {
      // this is last bucket
      // if the bucket is too large and this is the last bucket (i.e. wildcard),
      // unfold it.
      nextBucket();

      // the structure of the table has changed, so let's recheck if the peer
      // now has a dedicated bucket.
      auto resizedBucketId = getBucketId(buckets_, cpl);
      auto &resizedBucket = buckets_.at(resizedBucketId);
      if (resizedBucket.size() < config_.maxBucketSize) {
        resizedBucket.emplaceToFront(pid, !is_permanent);
        bus_->getChannel<events::PeerAddedChannel>().publish(pid);
        return true;
      }

      return replacePeer(resizedBucket, pid, !is_permanent, *bus_);
    }

    return replacePeer(bucket, pid, !is_permanent, *bus_);
  }

  void PeerRoutingTableImpl::nextBucket() {
    // This is the last bucket, which allegedly is a mixed bag containing
    // peers not belonging in dedicated (unfolded) buckets. _allegedly_ is
    // used here to denote that *all* peers in the last bucket might feasibly
    // belong to another bucket. This could happen if e.g. we've unfolded 4
    // buckets, and all peers in folded bucket 5 really belong in bucket 8.

    // get last bucket
    auto bucket = buckets_.rbegin();
    // split it
    auto newBucket = bucket->split(buckets_.size() - 1, local_);
    buckets_.push_back(std::move(newBucket));

    auto newLastBucket = buckets_.rbegin();
    if (newLastBucket->size() > config_.maxBucketSize) {
      nextBucket();
    }
  }

  size_t PeerRoutingTableImpl::size() const {
    return std::accumulate(
        buckets_.begin(), buckets_.end(), 0u,
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
}  // namespace libp2p::protocol::kademlia
