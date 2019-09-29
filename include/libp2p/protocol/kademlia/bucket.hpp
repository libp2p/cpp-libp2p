/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_BUCKET_HPP
#define LIBP2P_KAD_BUCKET_HPP

#include <deque>
#include <iostream>

#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>

namespace libp2p::protocol::kademlia {

  /**
   * @class Bucket
   *
   * Single bucket which holds peers.
   *
   * @see
   * https://sourcegraph.com/github.com/libp2p/go-libp2p-kbucket@HEAD/-/blob/bucket.go
   */
  class Bucket {
   public:
    using storage = std::deque<peer::PeerId>;
    using iterator = storage::iterator;
    using const_iterator = storage::const_iterator;

    iterator begin() {
      return bucket_.begin();
    }

    const_iterator begin() const {
      return bucket_.begin();
    }

    iterator end() {
      return bucket_.end();
    }

    const_iterator end() const {
      return bucket_.end();
    }

    void truncate(size_t size) {
      if (bucket_.size() > size) {
        bucket_.erase(std::next(bucket_.begin(), size), bucket_.end());
      }
    }

    std::vector<peer::PeerId> toVector() {
      return {bucket_.begin(), bucket_.end()};
    }

    void insertEnd(const_iterator begin, const_iterator end) {
      bucket_.insert(bucket_.end(), begin, end);
    }

    const storage &peers() const {
      return bucket_;
    }

    bool has(const peer::PeerId &p) const {
      auto it = std::find(bucket_.begin(), bucket_.end(), p);
      return it != bucket_.end();
    }

    bool remove(const peer::PeerId &p) {
      // this shifts elements to the end
      auto it = std::remove(bucket_.begin(), bucket_.end(), p);
      if (it != bucket_.end()) {
        bucket_.erase(it);
        return true;
      }

      return false;
    }

    void moveToFront(const peer::PeerId &p) {
      remove(p);
      pushFront(p);
    }

    void pushFront(const peer::PeerId &p) {
      bucket_.push_front(p);
    }

    peer::PeerId popBack() {
      peer::PeerId back = bucket_.back();
      bucket_.pop_back();
      return back;
    }

    size_t size() const {
      return bucket_.size();
    }

    bool empty() const {
      return bucket_.empty();
    }

    Bucket split(size_t commonLenPrefix, const NodeId &target) {
      Bucket b{};
      // remove shifts all elements "to be removed" to the end, other elements
      // preserve their relative order
      auto new_end = std::remove_if(
          bucket_.begin(), bucket_.end(), [&](const peer::PeerId &pid) {
            NodeId nodeId(pid);
            return nodeId.commonPrefixLen(target) > commonLenPrefix;
          });

      // bulk load "shifted" elements to new bucket
      b.insertEnd(new_end, bucket_.end());

      // remove "shifted" elements from
      bucket_.erase(new_end, bucket_.end());

      return b;
    }

   private:
    storage bucket_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_BUCKET_HPP
