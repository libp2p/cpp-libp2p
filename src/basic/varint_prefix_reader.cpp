/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/varint_prefix_reader.hpp>

namespace libp2p::basic {

  namespace {
    constexpr uint8_t kHighBitMask = 0x80;

    // just because 64 == 9*7 + 1
    constexpr uint8_t kMaxBytes = 10;

  }  // namespace

  void VarintPrefixReader::reset() {
    value_ = 0;
    state_ = kUnderflow;
    got_bytes_ = 0;
  }

  VarintPrefixReader::State VarintPrefixReader::consume(uint8_t byte) {
    if (state_ == kUnderflow) {
      bool next_byte_needed = (byte & kHighBitMask) != 0;
      uint64_t tmp = byte & ~kHighBitMask;

      switch (++got_bytes_) {
        case 1:
          break;
        case kMaxBytes:
          if (tmp > 1 || next_byte_needed) {
            state_ = kOverflow;
            return state_;
          }
          [[fallthrough]];
        default:
          tmp <<= 7 * (got_bytes_ - 1);
          break;
      }

      value_ += tmp;
      if (!next_byte_needed) {
        state_ = kReady;
      }

    } else if (state_ == kReady) {
      return kError;
    }

    return state_;
  }

  VarintPrefixReader::State VarintPrefixReader::consume(
      gsl::span<const uint8_t> &buffer) {
    size_t consumed = 0;
    State s(state_);
    for (auto byte : buffer) {
      ++consumed;
      s = consume(byte);
      if (s != kUnderflow) {
        break;
      }
    }
    if (consumed > 0 && (s == kReady || s == kUnderflow)) {
      buffer = buffer.subspan(consumed);
    }
    return s;
  }

}  // namespace libp2p::basic
