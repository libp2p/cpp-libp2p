/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/poll.hpp>
#include <libp2p/common/types.hpp>

namespace libp2p::basic {

  struct Reader {
    virtual ~Reader() = default;

    /**
     * `this` reference is not captured.
     * @param buffer used synchronously, reference is not captured.
     * @return non-zero count of bytes read to `buffer`.
     * @return `std::nullopt` if reader is closed.
     */
    virtual PollOutcome<size_t> pollReadSome(PollWaker waker,
                                             BytesOut buffer) = 0;
  };

}  // namespace libp2p::basic
