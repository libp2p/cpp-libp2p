/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/common/types.hpp>

namespace libp2p::security::secio {

  /**
   * Handy representation of protobuf ExchangeMessage in SECIO 1.0.0
   *
   * @see secio::ExchangeMessageMarshaller
   */
  struct ExchangeMessage {
    Bytes epubkey;
    Bytes signature;
  };
}  // namespace libp2p::security::secio
