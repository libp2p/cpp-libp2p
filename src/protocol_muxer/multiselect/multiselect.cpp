/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/multiselect.hpp>

namespace libp2p::protocol_muxer {
  using peer::Protocol;

void Multiselect::selectOneOf(
    gsl::span<const peer::Protocol> supported_protocols,
    std::shared_ptr<basic::ReadWriter> connection,
    bool is_initiator,
    ProtocolMuxer::ProtocolHandlerFunc handler) {
  if (supported_protocols.empty()) {
    handler(MultiselectError::PROTOCOLS_LIST_EMPTY);
    return;
    }

    negotiate(connection, supported_protocols, is_initiator, handler);
  }

  void Multiselect::negotiate(
      const std::shared_ptr<basic::ReadWriter> &connection,
      gsl::span<const peer::Protocol> supported_protocols,
      bool is_initiator,
      const ProtocolHandlerFunc &handler) {
    auto [write_buffer, read_buffer, index] = getBuffers();

    if (is_initiator) {
      MessageWriter::sendOpeningMsg(std::make_shared<ConnectionState>(
          connection,
          std::make_shared<std::vector<peer::Protocol>>(
              supported_protocols.begin(), supported_protocols.end()),
          handler,
          write_buffer,
          read_buffer,
          index,
          shared_from_this()));
    } else {
      MessageReader::readNextMessage(std::make_shared<ConnectionState>(
          connection,
          std::make_shared<std::vector<peer::Protocol>>(
              supported_protocols.begin(), supported_protocols.end()),
          handler,
          write_buffer,
          read_buffer,
          index,
          shared_from_this(),
          ConnectionState::NegotiationStatus::NOTHING_SENT));
    }
  }

  void Multiselect::negotiationRoundFailed(
      const std::shared_ptr<ConnectionState> &connection_state,
      const std::error_code &ec) {
    connection_state->proto_callback(ec);
    clearResources(connection_state);
  }

  void Multiselect::onWriteCompleted(
      std::shared_ptr<ConnectionState> connection_state) const {
    MessageReader::readNextMessage(std::move(connection_state));
  }

  void Multiselect::onWriteAckCompleted(
      const std::shared_ptr<ConnectionState> &connection_state,
      const Protocol &protocol) {
    negotiationRoundFinished(connection_state, protocol);
  }

  void Multiselect::onReadCompleted(
      std::shared_ptr<ConnectionState> connection_state,
      MessageManager::MultiselectMessage msg) {
    using MessageType = MessageManager::MultiselectMessage::MessageType;

    switch (msg.type) {
      case MessageType::OPENING:
        return handleOpeningMsg(std::move(connection_state));
      case MessageType::PROTOCOL:
        return handleProtocolMsg(msg.protocols[0], connection_state);
      case MessageType::PROTOCOLS:
        return handleProtocolsMsg(msg.protocols, connection_state);
      case MessageType::LS:
        return handleLsMsg(connection_state);
      case MessageType::NA:
        return handleNaMsg(connection_state);
      default:
        log_->critical(
            "type of the message, returned by the parser, is unknown");
        return negotiationRoundFailed(connection_state,
                                      MultiselectError::INTERNAL_ERROR);
    }
  }

  void Multiselect::handleOpeningMsg(
      std::shared_ptr<ConnectionState> connection_state) {
    using Status = ConnectionState::NegotiationStatus;

    switch (connection_state->status) {
      case Status::NOTHING_SENT:
        // we received an opening as a first message in this round; respond with
        // an opening as well
        return MessageWriter::sendOpeningMsg(std::move(connection_state));
      case Status::OPENING_SENT:
        // if opening is received as a response to ours, we can send ls to see
        // available protocols
        return MessageWriter::sendLsMsg(connection_state);
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::LS_SENT:
      case Status::NA_SENT:
        return onUnexpectedRequestResponse(connection_state);
      default:
        return onGarbagedStreamStatus(connection_state);
    }
  }

  void Multiselect::handleProtocolMsg(
      const peer::Protocol &protocol,
      const std::shared_ptr<ConnectionState> &connection_state) {
    using Status = ConnectionState::NegotiationStatus;

    switch (connection_state->status) {
      case Status::OPENING_SENT:
        return onProtocolAfterOpeningOrLs(connection_state, protocol);
      case Status::PROTOCOL_SENT:
        // this is ack that the protocol we want to communicate over is
        // supported by the other side; round is finished
        return negotiationRoundFinished(connection_state, protocol);
      case Status::PROTOCOLS_SENT:
        // the other side has chosen a protocol to communicate over; send an
        // ack, and round is finished
        return MessageWriter::sendProtocolAck(connection_state, protocol);
      case Status::LS_SENT:
        return onProtocolAfterOpeningOrLs(connection_state, protocol);
      case Status::NOTHING_SENT:
      case Status::NA_SENT:
        return onUnexpectedRequestResponse(connection_state);
      default:
        return onGarbagedStreamStatus(connection_state);
    }
  }

  void Multiselect::handleProtocolsMsg(
      const std::vector<Protocol> &protocols,
      const std::shared_ptr<ConnectionState> &connection_state) {
    using Status = ConnectionState::NegotiationStatus;

    switch (connection_state->status) {
      case Status::OPENING_SENT:
      case Status::PROTOCOL_SENT:
      case Status::PROTOCOLS_SENT:
      case Status::NA_SENT:
        return onUnexpectedRequestResponse(connection_state);
      case Status::LS_SENT:
        return onProtocolsAfterLs(connection_state, protocols);
      default:
        return onGarbagedStreamStatus(connection_state);
    }
  }

  void Multiselect::handleLsMsg(
      const std::shared_ptr<ConnectionState> &connection_state) {
    // respond with a list of protocols, supported by us
    auto protocols_to_send = connection_state->protocols;
    if (protocols_to_send->empty()) {
      return negotiationRoundFailed(connection_state,
                                    MultiselectError::INTERNAL_ERROR);
    }
    MessageWriter::sendProtocolsMsg(*protocols_to_send, connection_state);
  }

  void Multiselect::handleNaMsg(
      const std::shared_ptr<ConnectionState> &connection_state) const {
    // if we receive na message, just send an ls to understand, which protocols
    // the other side supports
    MessageWriter::sendLsMsg(connection_state);
  }

  void Multiselect::onProtocolAfterOpeningOrLs(
      std::shared_ptr<ConnectionState> connection_state,
      const peer::Protocol &protocol) {
    // the other side wants to communicate over that protocol; if it's available
    // on our side, round is finished
    auto protocols_to_search = connection_state->protocols;
    if (protocols_to_search->empty()) {
      return negotiationRoundFailed(connection_state,
                                    MultiselectError::INTERNAL_ERROR);
    }
    if (std::find(
            protocols_to_search->begin(), protocols_to_search->end(), protocol)
        != protocols_to_search->end()) {
      return MessageWriter::sendProtocolAck(std::move(connection_state),
                                            protocol);
    }

    // if the protocol is not available, send na
    MessageWriter::sendNaMsg(connection_state);
  }

  void Multiselect::onProtocolsAfterLs(
      const std::shared_ptr<ConnectionState> &connection_state,
      gsl::span<const peer::Protocol> received_protocols) {
    // if any of the received protocols is supported by our side, choose it;
    // fail otherwise
    auto protocols_to_search = connection_state->protocols;
    for (const auto &proto : *protocols_to_search) {
      // as size of vectors should be around 10 or less, we can use O(n*n)
      // approach
      if (std::find(received_protocols.begin(), received_protocols.end(), proto)
          != received_protocols.end()) {
        // the protocol is found
        return MessageWriter::sendProtocolMsg(proto, connection_state);
      }
    }

    negotiationRoundFailed(connection_state,
                           MultiselectError::NEGOTIATION_FAILED);
  }

  void Multiselect::onUnexpectedRequestResponse(
      const std::shared_ptr<ConnectionState> &connection_state) {
    log_->info("got a unexpected request-response combination - sending 'ls'");
    negotiationRoundFailed(connection_state,
                           MultiselectError::PROTOCOL_VIOLATION);
  }

  void Multiselect::onGarbagedStreamStatus(
      const std::shared_ptr<ConnectionState> &connection_state) {
    log_->critical("there is some garbage in stream state status");
    negotiationRoundFailed(connection_state, MultiselectError::INTERNAL_ERROR);
  }

  void Multiselect::negotiationRoundFinished(
      const std::shared_ptr<ConnectionState> &connection_state,
      const Protocol &chosen_protocol) {
    connection_state->proto_callback(chosen_protocol);
    clearResources(connection_state);
  }

  std::tuple<std::shared_ptr<common::ByteArray>,
             std::shared_ptr<boost::asio::streambuf>,
             size_t>
  Multiselect::getBuffers() {
    if (!free_buffers_.empty()) {
      auto free_buffers_index = free_buffers_.front();
      free_buffers_.pop();
      return {write_buffers_[free_buffers_index],
              read_buffers_[free_buffers_index],
              free_buffers_index};
    }
    return {
        write_buffers_.emplace_back(std::make_shared<common::ByteArray>()),
        read_buffers_.emplace_back(std::make_shared<boost::asio::streambuf>()),
        write_buffers_.size() - 1};
  }

  void Multiselect::clearResources(
      const std::shared_ptr<ConnectionState> &connection_state) {
    // add them to the pool of free buffers
    free_buffers_.push(connection_state->buffers_index);
  }
}  // namespace libp2p::protocol_muxer
