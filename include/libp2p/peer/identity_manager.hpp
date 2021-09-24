/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_IDENTITY_MANAGER_HPP
#define LIBP2P_IDENTITY_MANAGER_HPP

#include <libp2p/crypto/key.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace libp2p::event::peer {

  using KeyPairChangedChannel =
      channel_decl<struct KeyPairChanged, crypto::KeyPair>;

}  // namespace libp2p::event::peer

namespace libp2p::peer {

  /**
   * @brief Component, which "owns" information about current Host identity.
   */
  struct IdentityManager {
    virtual ~IdentityManager() = default;

    virtual const peer::PeerId &getId() const = 0;

    virtual const crypto::KeyPair &getKeyPair() const = 0;
  };

}  // namespace libp2p::peer

#endif  // LIBP2P_IDENTITY_MANAGER_HPP
