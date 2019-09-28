/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/key_repository/inmem_key_repository.hpp>

#include <libp2p/peer/errors.hpp>

namespace libp2p::peer {

  InmemKeyRepository::InmemKeyRepository()
      : kp_(std::make_shared<std::unordered_set<crypto::KeyPair>>()){};

  void InmemKeyRepository::clear(const PeerId &p) {
    auto it1 = pub_.find(p);
    if (it1 != pub_.end()) {
      // if vector is found, then clear it
      it1->second->clear();
    }

    kp_->clear();
  }

  outcome::result<InmemKeyRepository::PubVecPtr>
  InmemKeyRepository::getPublicKeys(const PeerId &p) {
    auto it = pub_.find(p);
    if (it == pub_.end()) {
      return PeerError::NOT_FOUND;
    }

    return it->second;
  }

  outcome::result<void> InmemKeyRepository::addPublicKey(const PeerId &p,
                                                         const Pub &pub) {
    auto it = pub_.find(p);
    if (it != pub_.end()) {
      // vector if sound
      PubVecPtr &ptr = it->second;
      ptr->insert(pub);
      return outcome::success();
    }

    // pub_[p] = [pub]
    auto ptr = std::make_shared<PubVec>();
    ptr->insert(pub);
    pub_.insert({p, std::move(ptr)});

    return outcome::success();
  }

  outcome::result<InmemKeyRepository::KeyPairVecPtr>
  InmemKeyRepository::getKeyPairs() {
    return kp_;
  };

  outcome::result<void> InmemKeyRepository::addKeyPair(const KeyPair &kp) {
    kp_->insert(kp);
    return outcome::success();
  }

  std::unordered_set<PeerId> InmemKeyRepository::getPeers() const {
    std::unordered_set<PeerId> peers;
    for (const auto &it : pub_) {
      peers.insert(it.first);
    }

    return peers;
  };

}  // namespace libp2p::peer
