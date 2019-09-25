/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ECHO_CONFIG_HPP
#define KAGOME_ECHO_CONFIG_HPP

#include <cstddef>

namespace libp2p::protocol {

  struct EchoConfig {
    // maximum size of message we can receive
    size_t max_recv_size = 4096;
  };

}  // namespace libp2p::protocol

#endif  // KAGOME_ECHO_CONFIG_HPP
