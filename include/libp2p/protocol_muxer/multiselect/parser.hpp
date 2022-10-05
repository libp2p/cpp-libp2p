/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTISELECT_PARSER_HPP
#define LIBP2P_MULTISELECT_PARSER_HPP

#include <libp2p/basic/read_buffer.hpp>
#include <libp2p/basic/varint_prefix_reader.hpp>

#include "common.hpp"

namespace libp2p::protocol_muxer::multiselect::detail {

  /// Multiselect message parser,
  /// Logic is similar to that of VarintPrefixReader
  class Parser {
    using VarintPrefixReader = basic::VarintPrefixReader;

   public:
    Parser() = default;

    /// Number of messages in a packet will rarely exceed 4
    using Messages = boost::container::small_vector<Message, 4>;

    using IndexType = gsl::span<const uint8_t>::index_type;

    /// State similar to that of VarintPrefixReader
    enum State {
      // using enum is possible only in c++20

      kUnderflow = VarintPrefixReader::kUnderflow,
      kReady = VarintPrefixReader::kReady,
      kOverflow = VarintPrefixReader::kOverflow,
      kError = VarintPrefixReader::kError,
    };

    /// Current state
    State state() const {
      return state_;
    }

    /// Returns protocol messages parsed
    const Messages &messages() const {
      return messages_;
    }

    /// Returs number of bytes needed for the next read operation
    size_t bytesNeeded() const;

    /// Resets the state and gets ready to read a new message
    void reset();

    /// Consumes incoming data from wire and returns state
    State consume(gsl::span<const uint8_t> &data);

   private:
    /// Called from consume() when length prefix is read
    void consumeData(gsl::span<const uint8_t> &data);

    /// Processes received packet, which can contain nested messages
    void readFinished(gsl::span<const uint8_t> msg);

    /// Parses nested messages (also varint prefixed)
    void parseNestedMessages(gsl::span<const uint8_t> &data);

    /// Processes received messages: assigns their types
    void processReceivedMessages();

    /// Ctor for nested messages parsing, called from inside only
    explicit Parser(size_t depth) : recursion_depth_(depth) {}

    /// Messages parsed
    Messages messages_;

    /// Collects message data, allocates from heap only if partial data received
    basic::FixedBufferCollector msg_buffer_;

    /// State. Initial stzte is kUnderflow
    State state_ = kUnderflow;

    /// Reader of length prefixes
    VarintPrefixReader varint_reader_;

    /// Message size expected as per length prefix
    IndexType expected_msg_size_ = 0;

    /// Recursion depth for nested messages, limited
    size_t recursion_depth_ = 0;
  };

}  // namespace libp2p::protocol_muxer::multiselect::detail

#endif  // LIBP2P_MULTISELECT_PARSER_HPP
