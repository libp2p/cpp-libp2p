/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECIO_PROPOSE_MESSAGE_HPP
#define LIBP2P_SECIO_PROPOSE_MESSAGE_HPP

#include <libp2p/common/types.hpp>

namespace libp2p::security::secio {

  /**
   * Handy representation of protobuf ProposeMessage in SECIO 1.0.0
   *
   * @see secio::ProposeMessageMarshaller
   */
  struct ProposeMessage {
    common::ByteArray rand;
    common::ByteArray pubkey;
    std::string exchanges;
    std::string ciphers;
    std::string hashes;
  };

}  // namespace libp2p::security::secio

#endif  // LIBP2P_SECIO_PROPOSE_MESSAGE_HPP
