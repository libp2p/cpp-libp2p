/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PEER_REPOSITORY_HPP
#define LIBP2P_PEER_REPOSITORY_HPP

#include <memory>

#include <libp2p/peer/address_repository.hpp>
#include <libp2p/peer/key_repository.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/protocol_repository.hpp>

namespace libp2p::peer {
  /**
   * @brief Repository which stores all known information about peers, including
   * this peer.
   */
  struct PeerRepository {
    virtual ~PeerRepository() = default;

    /**
     * @brief Getter for an address repository.
     * @return associated instance of an address repository.
     */
    virtual AddressRepository &getAddressRepository() = 0;

    /**
     * @brief Getter for a key repository.
     * @return associated instance of a key repository
     */
    virtual KeyRepository &getKeyRepository() = 0;

    /**
     * @brief Getter for a protocol repository.
     * @return associated instance of a protocol repository.
     */
    virtual ProtocolRepository &getProtocolRepository() = 0;

    /**
     * @brief Returns set of peer ids known by this peer repository.
     * @return unordered set of peers
     */
    virtual std::unordered_set<PeerId> getPeers() const = 0;

    /**
     * @brief Derive a PeerInfo object from the PeerId; can be useful, for
     * example, to establish connections, when only a PeerId is known at the
     * current program point
     * @param peer_id to get PeerInfo for
     * @return PeerInfo
     */
    virtual PeerInfo getPeerInfo(const PeerId &peer_id) const = 0;
  };
}  // namespace libp2p::peer

#endif  // LIBP2P_PEER_REPOSITORY_HPP
