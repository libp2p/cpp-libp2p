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
        log_("kad", "PeerRoutingTable") {
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
        bucket.insert(bucket.end(), left.begin(), left.end());
      }
      if (bucketId < buckets_.size() - 1) {
        auto &right = buckets_.at(bucketId + 1);
        bucket.insert(bucket.end(), right.begin(), right.end());
      }
    }

    // sort bucket in ascending order by XOR distance from local peer.
    XorDistanceComparator cmp(node_id);
    std::sort(bucket.begin(), bucket.end(), cmp);

    bucket.truncate(count);

    return bucket.toVector();
  }

  outcome::result<void> PeerRoutingTableImpl::update(const peer::PeerId &pid) {
    NodeId nodeId(pid);
    size_t cpl = nodeId.commonPrefixLen(local_);

    auto bucketId = getBucketId(buckets_, cpl);
    auto &bucket = buckets_.at(bucketId);

    // Trying to find and move to front
    auto it = std::remove(bucket.rbegin(), bucket.rend(), pid);
    if (it != bucket.rend()) {
      *it = pid;
      return outcome::success();
    }

    if (bucket.size() < config_.maxBucketSize) {
      bucket.push_front(pid);
      bus_->getChannel<events::PeerAddedChannel>().publish(pid);
      return outcome::success();
    }

    if (bucketId == buckets_.size() - 1) {
      // this is last bucket
      // if the bucket is too large and this is the last bucket (i.e. wildcard),
      // unfold it.
      nextBucket();

      // the structure of the table has changed, so let's recheck if the peer
      // now has a dedicated bucket.
      auto bid = cpl;
      if (bid >= buckets_.size()) {
        bid = buckets_.size() - 1;
      }

      auto &lastBucket = buckets_.at(bid);
      if (lastBucket.size() >= config_.maxBucketSize) {
        return Error::PEER_REJECTED_NO_CAPACITY;
      }

      lastBucket.push_front(pid);
      bus_->getChannel<events::PeerAddedChannel>().publish(pid);
      return outcome::success();
    }

    return Error::PEER_REJECTED_NO_CAPACITY;
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
    for (auto &bucket : buckets_) {
      vec.insert(vec.end(), bucket.begin(), bucket.end());
    }
    return vec;
  }
}  // namespace libp2p::protocol::kademlia
