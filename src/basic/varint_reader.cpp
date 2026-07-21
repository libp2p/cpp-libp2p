/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/varint_reader.hpp>
#include <qtils/bytes.hpp>

namespace libp2p::basic {
  auto VarintReader::poll(PollWaker waker, Reader &reader) -> Result {
    while (true) {
      if (varint_.state() == VarintPrefixReader::kReady) {
        return Result{varint_.value()};
      }
      if (varint_.state() == VarintPrefixReader::kError) {
        return PollOutcomeError{};
      }
      qtils::BytesN<1> buffer;
      POLL_OK_TRY(reader.pollReadSome(waker, buffer));
      varint_.consume(buffer[0]);
    }
  }
}  // namespace libp2p::basic
