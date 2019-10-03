/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_MUXER_HPP
#define LIBP2P_PROTOCOL_MUXER_HPP

#include <memory>

#include <gsl/span>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/basic/readwriter.hpp>
#include <libp2p/peer/protocol.hpp>

namespace libp2p::protocol_muxer {
  /**
   * Allows to negotiate with the other side of the connection about the
   * protocols, which are going to be used in communication with it
   */
  class ProtocolMuxer {
   public:
    using ProtocolHandlerFunc =
        std::function<void(const outcome::result<peer::Protocol> &)>;
    /**
     * Select a protocol for a given connection
     * @param protocols - set of protocols, one of which should be chosen during
     * the negotiation
     * @param connection, for which the protocol is being chosen
     * @param is_initiator - true, if we initiated the connection and thus
     * taking lead in the Multiselect protocol; false otherwise
     * @param cb - callback for handling negotiated protocol
     * @return chosen protocol or error
     */
    virtual void selectOneOf(gsl::span<const peer::Protocol> protocols,
                             std::shared_ptr<basic::ReadWriter> connection,
                             bool is_initiator, ProtocolHandlerFunc cb) = 0;

    virtual ~ProtocolMuxer() = default;
  };
}  // namespace libp2p::protocol_muxer

#endif  // LIBP2P_PROTOCOL_MUXER_HPP
