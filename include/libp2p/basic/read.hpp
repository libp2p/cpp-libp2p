/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/reader.hpp>
#include <libp2p/basic/void.hpp>

namespace libp2p {
  /**
   * Read exactly `buffer.size()` bytes.
   */
  class Read {
   public:
    using Result = PollOutcome<Void>;
    Result poll(PollWaker waker, basic::Reader &reader, BytesOut buffer) {
      while (true) {
        if (read_ >= buffer.size()) {
          return Result{Void{}};
        }
        auto n = POLL_OK_TRY(reader.pollReadSome(waker, buffer.subspan(read_)));
        read_ += n;
      }
    }

   private:
    size_t read_ = 0;
  };
}  // namespace libp2p
