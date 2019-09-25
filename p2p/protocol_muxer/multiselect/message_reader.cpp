/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "protocol_muxer/multiselect/message_reader.hpp"

#include <boost/optional.hpp>
#include "multi/uvarint.hpp"
#include "protocol_muxer/multiselect/message_manager.hpp"
#include "protocol_muxer/multiselect/multiselect.hpp"

namespace {
  using libp2p::multi::UVarint;

  boost::optional<UVarint> getVarint(boost::asio::streambuf &buffer) {
    return UVarint::create(gsl::make_span(
        static_cast<const uint8_t *>(buffer.data().data()), buffer.size()));
  }
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
    // '/tls/1.3.0\n' - the shortest protocol, which could be found
    static constexpr size_t kShortestProtocolLength = 11;

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
    if (!const_msg_res
        && const_msg_res.error()
            != MessageManager::ParseError::MSG_IS_ILL_FORMED) {
      // MSG_IS_ILL_FORMED allows us to continue parsing; otherwise, it's an
      // error
      multiselect->negotiationRoundFailed(connection_state,
                                          const_msg_res.error());
      return;
    }

    // here, we assume that protocols header - two varints + '\n' - cannot be
    // longer or equal to the shortest protocol length; varints should be very
    // big for it to happen; thus, continue parsing depending on the length of
    // the current string
    if (read_bytes < kShortestProtocolLength) {
      auto proto_header_res = MessageManager::parseProtocolsHeader(msg_span);
      if (proto_header_res) {
        auto proto_header = proto_header_res.value();
        readNextBytes(connection_state, proto_header.size_of_protocols,
                      [proto_header](std::shared_ptr<ConnectionState> state) {
                        onReadProtocolsCompleted(
                            std::move(state), proto_header.size_of_protocols,
                            proto_header.number_of_protocols);
                      });
      } else {
        multiselect->negotiationRoundFailed(connection_state,
                                            proto_header_res.error());
      }
      return;
    }

    auto proto_res = MessageManager::parseProtocol(msg_span);
    if (!proto_res) {
      multiselect->negotiationRoundFailed(connection_state, proto_res.error());
      return;
    }

    multiselect->onReadCompleted(connection_state,
                                 std::move(proto_res.value()));
  }

  void MessageReader::onReadProtocolsCompleted(
      std::shared_ptr<ConnectionState> connection_state,
      uint64_t expected_protocols_size, uint64_t expected_protocols_number) {
    auto multiselect = connection_state->multiselect;

    auto msg_res = MessageManager::parseProtocols(
        gsl::make_span(static_cast<const uint8_t *>(
                           connection_state->read_buffer->data().data()),
                       expected_protocols_size),
        expected_protocols_number);
    connection_state->read_buffer->consume(expected_protocols_size);
    if (!msg_res) {
      multiselect->negotiationRoundFailed(connection_state, msg_res.error());
      return;
    }

    multiselect->onReadCompleted(std::move(connection_state),
                                 std::move(msg_res.value()));
  }

}  // namespace libp2p::protocol_muxer
