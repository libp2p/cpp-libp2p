/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KEY_REPOSITORY_HPP
#define LIBP2P_KEY_REPOSITORY_HPP

#include <unordered_set>
#include <vector>

#include <libp2p/crypto/key.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::peer {

  /**
   * @brief Class that provides access to public keys of other peers, and
   * keypairs of this peer.
   */
  class KeyRepository {
   protected:
    using Pub = crypto::PublicKey;
    using PubVec = std::unordered_set<Pub>;
    using PubVecPtr = std::shared_ptr<PubVec>;

    using KeyPair = crypto::KeyPair;
    using KeyPairVec = std::unordered_set<KeyPair>;
    using KeyPairVecPtr = std::shared_ptr<KeyPairVec>;

   public:
    virtual ~KeyRepository() = default;

    /**
     * @brief remove all keys related to peer {@param p}
     * @param p peer
     */
    virtual void clear(const PeerId &p) = 0;

    /**
     * @brief Getter for public keys of given {@param p} Peer.
     * @param p PeerId
     * @return pointer to a set of public keys associated with given peer.
     */
    virtual outcome::result<PubVecPtr> getPublicKeys(const PeerId &p) = 0;

    /**
     * @brief Add public key to a set of public keys associated with {@param p}
     * PeerId.
     * @param p PeerId
     * @param pub public key
     * @return error code in case of error.
     */
    virtual outcome::result<void> addPublicKey(
        const PeerId &p, const crypto::PublicKey &pub) = 0;

    /**
     * @brief Getter for keypairs associated with this peer.
     * @param p PeerId.
     * @return pointer to a set of keypairs associated with {@param p}.
     */
    virtual outcome::result<KeyPairVecPtr> getKeyPairs() = 0;

    /**
     * @brief Associate a keypair {@param kp} with current peer.
     * @param kp KeyPair
     * @return error code in case of error.
     */
    virtual outcome::result<void> addKeyPair(const KeyPair &kp) = 0;

    /**
     * @brief Returns set of peer ids known by this repository.
     * @return unordered set of peers
     */
    virtual std::unordered_set<PeerId> getPeers() const = 0;
  };

}  // namespace libp2p::peer

#endif  // LIBP2P_KEY_REPOSITORY_HPP
