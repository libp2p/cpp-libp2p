/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PLAINTEXT_EXCHANGE_MESSAGE_HPP
#define LIBP2P_PLAINTEXT_EXCHANGE_MESSAGE_HPP

#include <libp2p/crypto/key.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace libp2p::security::plaintext {

  /**
   * Protobuf exchange message used by Plaintext 2.0 protocol
   * Structures generated by Google Protobuf are rather inconvenient, thus use
   * this one.
   * @see ExchangeMessageMarshaller
   */
  struct ExchangeMessage {
    crypto::PublicKey pubkey;
    peer::PeerId peer_id;
  };

}  // namespace libp2p::security::plaintext

#endif  // LIBP2P_PLAINTEXT_EXCHANGE_MESSAGE_HPP
