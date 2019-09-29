/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECURE_CONNECTION_HPP
#define LIBP2P_SECURE_CONNECTION_HPP

#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/crypto/key.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace libp2p::connection {

  /**
   * @brief Class that represents a connection that is authenticated and
   * encrypted.
   */
  struct SecureConnection : public RawConnection {
    ~SecureConnection() override = default;

    /**
     * Get a PeerId of our local peer
     * @return peer id
     */
    virtual outcome::result<peer::PeerId> localPeer() const = 0;

    /**
     * Get a PeerId of peer this connection is established with
     * @return peer id
     */
    virtual outcome::result<peer::PeerId> remotePeer() const = 0;

    /**
     * Get a public key of peer this connection is established with
     * @return public key
     */
    virtual outcome::result<crypto::PublicKey> remotePublicKey() const = 0;

    // TODO(warchant): figure out, if it is needed
    // virtual crypto::PrivateKey localPrivateKey() const = 0;
  };

}  // namespace libp2p::connection

#endif  // LIBP2P_SECURE_CONNECTION_HPP
