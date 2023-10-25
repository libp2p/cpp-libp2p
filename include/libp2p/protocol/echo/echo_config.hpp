/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>

namespace libp2p::protocol {

  struct EchoConfig {
    static constexpr size_t kInfiniteNumberOfRepeats = 0;

    // number of times echo server will repeat the cycle <listen-respond>
    size_t max_server_repeats = kInfiniteNumberOfRepeats;

    // maximum size of message we can receive
    size_t max_recv_size = 4096;
  };

}  // namespace libp2p::protocol
