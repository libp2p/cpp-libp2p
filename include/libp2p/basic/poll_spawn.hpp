/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/poll.hpp>
#include <libp2p/common/shared_fn.hpp>

namespace libp2p {
  /**
   * Start polling lambda.
   * Provides `waker` which calls same lambda.
   */
  auto pollSpawn(auto &&f) {
    SharedFn f_shared{std::forward<decltype(f)>(f)};
    f_shared(PollWaker{f_shared});
  }
}  // namespace libp2p
