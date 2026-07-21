/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/reader.hpp>
#include <libp2p/basic/varint_prefix_reader.hpp>

namespace libp2p::basic {
  class VarintReader {
   public:
    using Result = PollOutcome<uint64_t>;
    Result poll(PollWaker waker, Reader &reader);

   private:
    VarintPrefixReader varint_;
  };
}  // namespace libp2p::basic
