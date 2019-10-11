/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MESSAGE_WRITER_HPP
#define LIBP2P_MESSAGE_WRITER_HPP

#include <functional>
#include <memory>

#include <gsl/span>
#include <libp2p/peer/protocol.hpp>
#include <libp2p/protocol_muxer/multiselect/connection_state.hpp>

namespace libp2p::protocol_muxer {
  class Multiselect;

  /**
   * Sends messages of Multiselect format
   */
  class MessageWriter {
   public:
    /**
     * Send a message, signalizing about start of the negotiation
     * @param connection_state - state of the connection
     */
    static void sendOpeningMsg(
        std::shared_ptr<ConnectionState> connection_state);

    /**
     * Send a message, containing a protocol
     * @param protocol to be sent
     * @param connection_state - state of the connection
     */
    static void sendProtocolMsg(
        const peer::Protocol &protocol,
        const std::shared_ptr<ConnectionState> &connection_state);

    /**
     * Send a message, containing protocols
     * @param protocols to be sent
     * @param connection_state - state of the connection
     */
    static void sendProtocolsMsg(
        gsl::span<const peer::Protocol> protocols,
        const std::shared_ptr<ConnectionState> &connection_state);

    /**
     * Send a message, containing an na
     * @param connection_state - state of the connection
     */
    static void sendNaMsg(
        const std::shared_ptr<ConnectionState> &connection_state);

    /**
     * Send an ack message for the chosen protocol
     * @param connection_state - state of the connection
     * @param protocol - chosen protocol
     */
    static void sendProtocolAck(
        std::shared_ptr<ConnectionState> connection_state,
        const peer::Protocol &protocol);

   private:
    /**
     * Get a callback to be used in connection write functions
     * @param connection_state - state of the connection
     * @param success_status - status to be set after a successful write
     * @return lambda-callback for the write operation
     */
    static auto getWriteCallback(
        std::shared_ptr<ConnectionState> connection_state,
        ConnectionState::NegotiationStatus success_status);
  };
}  // namespace libp2p::protocol_muxer

#endif  // LIBP2P_MESSAGE_WRITER_HPP
