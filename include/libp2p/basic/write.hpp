/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/void.hpp>
#include <libp2p/basic/writer.hpp>

namespace libp2p {
  /**
   * Write exactly `buffer.size()` bytes.
   */
  class Write {
   public:
    using Result = PollOutcome<Void>;
    Result poll(PollWaker waker, basic::Writer &writer, BytesIn buffer) {
      while (true) {
        if (written_ >= buffer.size()) {
          return Result{Void{}};
        }
        auto n =
            POLL_OK_TRY(writer.pollWriteSome(waker, buffer.subspan(written_)));
        written_ += n;
      }
    }

   private:
    size_t written_ = 0;
  };
}  // namespace libp2p
