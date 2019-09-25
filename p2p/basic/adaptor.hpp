/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADAPTOR_HPP
#define KAGOME_ADAPTOR_HPP

#include "peer/protocol.hpp"

namespace libp2p::basic {
  /**
   * Base class of adaptor - entity, which encapsulates logic of upgrading the
   * connection
   */
  struct Adaptor {
    virtual ~Adaptor() = default;

    /**
     * Get a string identifier, associated with this adaptor
     * @return protocol id of the adaptor
     * @example '/yamux/1.0.0' or '/plaintext/1.5.0'
     */
    virtual peer::Protocol getProtocolId() const = 0;
  };
}  // namespace libp2p::basic

#endif  // KAGOME_ADAPTOR_HPP
