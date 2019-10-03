/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PEER_ADDRESS_HPP
#define LIBP2P_PEER_ADDRESS_HPP

#include <memory>
#include <string>
#include <string_view>

#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::peer {
  /**
   * Address of the given peer; includes its ID and multiaddress; usually is
   * passed over the network
   */
  class PeerAddress {
    using FactoryResult = outcome::result<PeerAddress>;

   public:
    enum class FactoryError { ID_EXPECTED = 1, NO_ADDRESSES, SHA256_EXPECTED };

    /**
     * Create a PeerAddress from the string of format
     * "<multiaddress>/id/<base58_encoded_peer_id>"
     * @param address - stringified address, for instance,
     * "/ip4/192.168.0.1/tcp/1234/p2p/<ID>"
     * @return created object in case of success, error otherwise
     */
    static FactoryResult create(std::string_view address);

    /**
     * Create a PeerAddress from the PeerInfo structure
     * @param peer_info, from which address is to be created; MUST contain both
     * PeerId and at least one Multiaddress; if there are several addresses in
     * the structure, a random one is chosen
     * @return created object in case of success, error otherwise
     */
    static FactoryResult create(const PeerInfo &peer_info);

    /**
     * Create a PeerAddress from PeerId and Multiaddress
     * @param peer_id of the address to be created
     * @param address of the address to be created
     * @return created object in case of success, error otherwise
     */
    static FactoryResult create(const PeerId &peer_id,
                                const multi::Multiaddress &address);

    /**
     * Get a string representation of this address;
     * "<multiaddress>/p2p/<base58-peer-id>"
     * @return address string
     */
    std::string toString() const;

    /**
     * Get a PeerId in this address
     * @return peer id
     */
    const PeerId &getId() const noexcept;

    /**
     * Get a Multiaddress in this address
     * @return multiaddress
     */
    const multi::Multiaddress &getAddress() const noexcept;

   private:
    /**
     * Construct a PeerAddress instance
     * @param id, with which an instance is to be created
     * @param address, with which an instance is to be created
     */
    PeerAddress(PeerId id, multi::Multiaddress address);

    PeerId id_;
    multi::Multiaddress address_;
  };
}  // namespace libp2p::peer

OUTCOME_HPP_DECLARE_ERROR(libp2p::peer, PeerAddress::FactoryError)

#endif  // LIBP2P_PEER_ADDRESS_HPP
