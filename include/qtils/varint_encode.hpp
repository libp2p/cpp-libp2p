/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/bytes.hpp>

namespace qtils {
  /**
   * Encode varint (LEB128) to stack buffer.
   */
  class VarintEncode {
   public:
    VarintEncode(uint64_t value) {
      do {
        uint8_t byte = static_cast<uint8_t>(value) & 0x7f;
        value >>= 7;
        if (value != 0) {
          byte |= 0x80;
        }
        buffer_[size_] = byte;
        ++size_;
      } while (value != 0);
    }

    qtils::BytesIn buffer() const {
      return qtils::BytesIn{buffer_}.first(size_);
    }

   private:
    qtils::BytesN<(sizeof(uint64_t) * 8 + 6) / 7> buffer_;
    uint8_t size_ = 0;
  };
}  // namespace qtils
