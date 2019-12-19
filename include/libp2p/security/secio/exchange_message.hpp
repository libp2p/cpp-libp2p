/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECIO_EXCHANGE_MESSAGE_HPP
#define LIBP2P_SECIO_EXCHANGE_MESSAGE_HPP

#include <libp2p/common/types.hpp>

namespace libp2p::security::secio {

  /**
   * Handy representation of protobuf ExchangeMessage in SECIO 1.0.0
   *
   * @see secio::ExchangeMessageMarshaller
   */
  struct ExchangeMessage {
    common::ByteArray epubkey;
    common::ByteArray signature;
  };
}  // namespace libp2p::security::secio

#endif  // LIBP2P_SECIO_EXCHANGE_MESSAGE_HPP
