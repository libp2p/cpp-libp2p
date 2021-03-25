/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/message_writer.hpp>

#include <libp2p/protocol_muxer/multiselect/message_manager.hpp>
#include <libp2p/protocol_muxer/multiselect/multiselect.hpp>

namespace libp2p::protocol_muxer {
  using peer::Protocol;

  auto MessageWriter::getWriteCallback(
      std::shared_ptr<ConnectionState> connection_state,
      ConnectionState::NegotiationStatus success_status) {
    return [connection_state = std::move(connection_state), success_status](
               const outcome::result<size_t> written_bytes_res) mutable {
      auto multiselect = connection_state->multiselect;
      if (not written_bytes_res) {
        multiselect->negotiationRoundFailed(connection_state,
                                            written_bytes_res.error());
        return;
      }
      connection_state->status = success_status;
      multiselect->onWriteCompleted(std::move(connection_state));
    };
  }

  void MessageWriter::sendOpeningMsg(
      std::shared_ptr<ConnectionState> connection_state) {
    *connection_state->write_buffer = MessageManager::openingMsg();
    auto state = connection_state;
    state->write(
        getWriteCallback(std::move(connection_state),
                         ConnectionState::NegotiationStatus::OPENING_SENT));
  }

  void MessageWriter::sendProtocolMsg(
      const Protocol &protocol,
      const std::shared_ptr<ConnectionState> &connection_state) {
    *connection_state->write_buffer = MessageManager::protocolMsg(protocol);
    const auto &state = connection_state;
    state->write(getWriteCallback(
        connection_state, ConnectionState::NegotiationStatus::PROTOCOL_SENT));
  }

  void MessageWriter::sendProtocolsMsg(
      gsl::span<const Protocol> protocols,
      const std::shared_ptr<ConnectionState> &connection_state) {
    *connection_state->write_buffer = MessageManager::protocolsMsg(protocols);
    const auto &state = connection_state;
    state->write(getWriteCallback(
        connection_state, ConnectionState::NegotiationStatus::PROTOCOLS_SENT));
  }

  void MessageWriter::sendNaMsg(
      const std::shared_ptr<ConnectionState> &connection_state) {
    *connection_state->write_buffer = MessageManager::naMsg();
    const auto &state = connection_state;
    state->write(getWriteCallback(connection_state,
                                  ConnectionState::NegotiationStatus::NA_SENT));
  }

  void MessageWriter::sendProtocolAck(
      std::shared_ptr<ConnectionState> connection_state,
      const peer::Protocol &protocol) {
    *connection_state->write_buffer = MessageManager::protocolMsg(protocol);
    auto state = connection_state;
    state->write([connection_state = std::move(connection_state), protocol](
                     const outcome::result<size_t> written_bytes_res) mutable {
      auto multiselect = connection_state->multiselect;
      if (not written_bytes_res) {
        multiselect->negotiationRoundFailed(connection_state,
                                            written_bytes_res.error());
        return;
      }
      connection_state->status =
          ConnectionState::NegotiationStatus::PROTOCOL_SENT;
      multiselect->onWriteAckCompleted(connection_state, protocol);
    });
  }

}  // namespace libp2p::protocol_muxer
