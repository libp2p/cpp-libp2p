/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/poll.hpp>
#include <libp2p/common/types.hpp>

namespace libp2p::basic {

  struct Writer {
    virtual ~Writer() = default;

    /**
     * `this` reference is not captured.
     * @param buffer used synchronously, reference is not stored.
     * @return non-zero count of bytes written from `buffer`.
     * @return `std::nullopt` if writer is closed.
     */
    virtual PollOutcome<size_t> pollWriteSome(PollWaker waker,
                                              BytesIn buffer) = 0;
  };

}  // namespace libp2p::basic
