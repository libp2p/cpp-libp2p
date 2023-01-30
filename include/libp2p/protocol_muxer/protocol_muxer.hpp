/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_MUXER_HPP
#define LIBP2P_PROTOCOL_MUXER_HPP

#include <memory>

#include <gsl/span>
#include <libp2p/connection/stream.hpp>
#include <libp2p/peer/protocol.hpp>

namespace libp2p::protocol_muxer {
  /**
   * Allows to negotiate with the other side of the connection about the
   * protocols, which are going to be used in communication with it
   */
  class ProtocolMuxer {
   public:
    enum class Error {
      // cannot negotiate protocol
      NEGOTIATION_FAILED = 1,

      // error occured on this host's side
      INTERNAL_ERROR,

      // remote peer violated protocol
      PROTOCOL_VIOLATION,
    };

    using ProtocolHandlerFunc =
        std::function<void(outcome::result<peer::ProtocolName>)>;
    /**
     * Select a protocol for a given connection
     * @param protocols - set of protocols, one of which should be chosen during
     * the negotiation
     * @param connection, for which the protocol is being chosen
     * @param is_initiator - true, if we initiated the connection and thus
     * taking lead in the Multiselect protocol; false otherwise
     * @param negotiate_multistream - true, if we need to negotiate multistream
     * itself, this happens with fresh raw connections
     * @param cb - callback for handling negotiated protocol
     * @return chosen protocol or error
     */
    virtual void selectOneOf(gsl::span<const peer::ProtocolName> protocols,
                             std::shared_ptr<basic::ReadWriter> connection,
                             bool is_initiator, bool negotiate_multistream,
                             ProtocolHandlerFunc cb) = 0;

    /**
     * Simple (Yes/No) negotiation of a single protocol on a fresh outbound
     * stream
     * @param stream Stream, just connected
     * @param protocol_id Protocol to negotiate
     * @param cb Stream result external callback
     */
    virtual void simpleStreamNegotiate(
        const std::shared_ptr<connection::Stream> &stream,
        const peer::ProtocolName &protocol_id,
        std::function<
            void(outcome::result<std::shared_ptr<connection::Stream>>)>
            cb) = 0;

    virtual ~ProtocolMuxer() = default;
  };
}  // namespace libp2p::protocol_muxer

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol_muxer, ProtocolMuxer::Error)

#endif  // LIBP2P_PROTOCOL_MUXER_HPP
