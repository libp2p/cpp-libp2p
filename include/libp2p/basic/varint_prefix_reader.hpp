/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_VARINT_PREFIX_READER_HPP
#define LIBP2P_VARINT_PREFIX_READER_HPP

#include <gsl/span>

namespace libp2p::basic {

  /// Collects and interprets varint from incoming data,
  /// Reader is stateful, see varint_prefix_reader_test.cpp for usage examples
  class VarintPrefixReader {
   public:
    /// Current state
    enum State {
      /// Needs more bytes
      kUnderflow,

      /// Varint is ready, value() is ultimate
      kReady,

      /// Overflow of uint64_t, too many bytes with high bit set
      kOverflow,

      /// consume() called when state is kReady
      kError
    };

    /// Returns state
    State state() const {
      return state_;
    }

    /// Returns current value, called when state() == kReady
    uint64_t value() const {
      return value_;
    }

    /// Resets reader's state
    void reset();

    /// Consumes one byte from wire, returns reader's state
    /// (or kError if called when state() == kReady)
    State consume(uint8_t byte);

    /// Consumes bytes from buffer.
    /// On success, modifies buffer (cuts off first bytes which were consumed),
    /// returns reader's state
    /// (or kError if called when state() == kReady)
    State consume(gsl::span<const uint8_t> &buffer);

   private:
    /// Current value accumulated
    uint64_t value_ = 0;

    /// Current reader's state
    State state_ = kUnderflow;

    /// Bytes got at the moment, this controls overflow of value_
    uint8_t got_bytes_ = 0;
  };
}  // namespace libp2p::basic

#endif  // LIBP2P_VARINT_PREFIX_READER_HPP
