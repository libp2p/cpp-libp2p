/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/message_reader.hpp>

#include <boost/optional.hpp>
#include <libp2p/multi/uvarint.hpp>
#include <libp2p/protocol_muxer/multiselect/message_manager.hpp>
#include <libp2p/protocol_muxer/multiselect/multiselect.hpp>

namespace {
  using libp2p::multi::UVarint;

  boost::optional<UVarint> getVarint(boost::asio::streambuf &buffer) {
    return UVarint::create(gsl::make_span(
        static_cast<const uint8_t *>(buffer.data().data()), buffer.size()));
  }

  // in Reader's code we want this header without '\n' char
  using libp2p::protocol_muxer::MessageManager;
  const std::string_view kMultiselectHeader =
      MessageManager::kMultiselectHeader.substr(
          0, MessageManager::kMultiselectHeader.size() - 1);
}  // namespace

namespace libp2p::protocol_muxer {
  void MessageReader::readNextMessage(
      std::shared_ptr<ConnectionState> connection_state) {
    readNextVarint(std::move(connection_state));
  }

  void MessageReader::readNextVarint(
      std::shared_ptr<ConnectionState> connection_state) {
    // we don't know exact length of varint, so read byte-by-byte
    auto state = connection_state;
    state->read(1,
                [connection_state = std::move(connection_state)](
                    const outcome::result<void> &res) mutable {
                  if (not res) {
                    auto multiselect = connection_state->multiselect;
                    multiselect->negotiationRoundFailed(
                        connection_state, MultiselectError::INTERNAL_ERROR);
                    return;
                  }
                  onReadVarintCompleted(std::move(connection_state));
                });
  }

  void MessageReader::onReadVarintCompleted(
      std::shared_ptr<ConnectionState> connection_state) {
    auto varint_opt = getVarint(*connection_state->read_buffer);
    if (!varint_opt) {
      // no varint; continue reading
      readNextVarint(std::move(connection_state));
      return;
    }
    // we have length of the line to be read; do it
    connection_state->read_buffer->consume(varint_opt->size());

    auto bytes_to_read = varint_opt->toUInt64();
    readNextBytes(std::move(connection_state), bytes_to_read,
                  [bytes_to_read](auto &&state) {
                    onReadLineCompleted(std::forward<decltype(state)>(state),
                                        bytes_to_read);
                  });
  }

  void MessageReader::readNextBytes(
      std::shared_ptr<ConnectionState> connection_state, uint64_t bytes_to_read,
      std::function<void(std::shared_ptr<ConnectionState>)> final_callback) {
    const auto &state = connection_state;
    state->read(bytes_to_read,
                [connection_state = std::move(connection_state),
                 final_callback = std::move(final_callback)](
                    const outcome::result<void> &res) mutable {
                  if (not res) {
                    auto multiselect = connection_state->multiselect;
                    multiselect->negotiationRoundFailed(
                        connection_state, MultiselectError::INTERNAL_ERROR);
                    return;
                  }
                  final_callback(std::move(connection_state));
                });
  }

  void MessageReader::onReadLineCompleted(
      const std::shared_ptr<ConnectionState> &connection_state,
      uint64_t read_bytes) {
    using Message = MessageManager::MultiselectMessage;

    auto multiselect = connection_state->multiselect;

    auto msg_span =
        gsl::make_span(static_cast<const uint8_t *>(
                           connection_state->read_buffer->data().data()),
                       read_bytes);
    connection_state->read_buffer->consume(msg_span.size());

    // firstly, try to match the message against constant messages
    auto const_msg_res = MessageManager::parseConstantMsg(msg_span);
    if (const_msg_res) {
      multiselect->onReadCompleted(connection_state,
                                   std::move(const_msg_res.value()));
      return;
    }
    if (const_msg_res.error()
        != MessageManager::ParseError::MSG_IS_ILL_FORMED) {
      // MSG_IS_ILL_FORMED allows us to continue parsing; otherwise, it's an
      // error
      multiselect->negotiationRoundFailed(connection_state,
                                          const_msg_res.error());
      return;
    }

    // if it's not a constant message, it contains one or more protocols;
    // firstly assume the first case - it contains one protocol - we can just
    // parse it till the '\n' char, and if length of this parsed protocol + 1
    // (for the '\n') is equal to the length of the read message, it's a
    // one-protocol message; if not, parse it as a several-protocols message
    auto parsed_protocol_res = MessageManager::parseProtocol(msg_span);
    if (parsed_protocol_res
        && (parsed_protocol_res.value().size() + 1)
            == static_cast<size_t>(msg_span.size())) {
      // it's a single-protocol message; check against an opening protocol
      auto parsed_protocol = std::move(parsed_protocol_res.value());
      if (parsed_protocol == kMultiselectHeader) {
        return multiselect->onReadCompleted(
            connection_state, Message{Message::MessageType::OPENING});
      }
      return multiselect->onReadCompleted(
          connection_state,
          Message{Message::MessageType::PROTOCOL, {parsed_protocol}});
    }

    // it's a several-protocols message
    auto protocols_msg_res = MessageManager::parseProtocols(msg_span);
    if (!protocols_msg_res) {
      // message cannot be parsed
      return multiselect->negotiationRoundFailed(connection_state,
                                          protocols_msg_res.error());
    }

    multiselect->onReadCompleted(connection_state, std::move(protocols_msg_res.value()));
  }

}  // namespace libp2p::protocol_muxer
