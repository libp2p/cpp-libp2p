/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/routing_table_impl.hpp>

#include <numeric>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol::kademlia, RoutingTableImpl::Error,
                            e) {
  using E = libp2p::protocol::kademlia::RoutingTableImpl::Error;

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

  size_t getBucketId(std::vector<Bucket> &buckets, size_t cpl) {
    BOOST_ASSERT(!buckets.empty());
    return cpl >= buckets.size() ? buckets.size() - 1 : cpl;
  }
}  // namespace

namespace libp2p::protocol::kademlia {

  void RoutingTableImpl::remove(const peer::PeerId &id) {
    NodeId nodeId(id);
    size_t cpl = nodeId.commonPrefixLen(local_);
    auto bucketId = getBucketId(buckets_, cpl);
    auto &bucket = buckets_.at(bucketId);
    bucket.remove(id);
    bus_->getChannel<events::PeerRemovedChannel>().publish(id);
  }

  PeerIdVec RoutingTableImpl::getNearestPeers(const NodeId &id, size_t count) {
    size_t cpl = id.commonPrefixLen(local_);
    size_t bucketId = getBucketId(buckets_, cpl);

    // make a copy of a bucket
    auto bucket = buckets_.at(bucketId);
    if (bucket.size() < count) {
      // In the case of an unusual split, one bucket may be short or empty.
      // if this happens, search both surrounding buckets for nearby peers
      if (bucketId > 0) {
        auto &left = buckets_.at(bucketId - 1);
        bucket.insertEnd(left.begin(), left.end());
      }
      if (bucketId < buckets_.size() - 1) {
        auto &right = buckets_.at(bucketId + 1);
        bucket.insertEnd(right.begin(), right.end());
      }
    }

    // sort bucket in ascending order by XOR distance from local peer.
    XorDistanceComparator cmp(id);
    std::sort(bucket.begin(), bucket.end(), cmp);

    if (count < bucket.size()) {
      bucket.truncate(count);
    }

    return bucket.toVector();
  }

  outcome::result<void> RoutingTableImpl::update(const peer::PeerId &pid) {
    NodeId nodeId(pid);
    size_t cpl = nodeId.commonPrefixLen(local_);

    auto bucketId = getBucketId(buckets_, cpl);
    auto &bucket = buckets_.at(bucketId);
    if (bucket.has(pid)) {
      bucket.moveToFront(pid);
      return outcome::success();
    }

    // TODO(warchant): check for latency here
    // https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/table.go#L81

    if (bucket.size() < bucket_size_) {
      bucket.pushFront(pid);
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
      if (lastBucket.size() >= bucket_size_) {
        return Error::PEER_REJECTED_NO_CAPACITY;
      }

      lastBucket.pushFront(pid);
      bus_->getChannel<events::PeerAddedChannel>().publish(pid);
      return outcome::success();
    }

    return Error::PEER_REJECTED_NO_CAPACITY;
  }

  void RoutingTableImpl::nextBucket() {
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
    if (newLastBucket->size() > bucket_size_) {
      nextBucket();
    }
  }

  size_t RoutingTableImpl::size() const {
    return std::accumulate(
        buckets_.begin(), buckets_.end(), 0u,
        [](size_t num, const Bucket &bucket) { return num + bucket.size(); });
  }

  PeerIdVec RoutingTableImpl::getAllPeers() const {
    PeerIdVec vec;
    for (auto &bucket : buckets_) {
      vec.insert(vec.end(), bucket.begin(), bucket.end());
    }
    return vec;
  }

  RoutingTableImpl::RoutingTableImpl(
      std::shared_ptr<peer::IdentityManager> idmgr,
      std::shared_ptr<event::Bus> bus, const KademliaConfig &config)
      : idmgr_(std::move(idmgr)),
        local_(idmgr_->getId()),
        bus_(std::move(bus)),
        bucket_size_(config.K) {
    BOOST_ASSERT(idmgr_ != nullptr);
    BOOST_ASSERT(bus_ != nullptr);
    buckets_.emplace_back();  // create 1 bucket
  }

}  // namespace libp2p::protocol::kademlia
