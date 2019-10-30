/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/network/default_network.hpp>

namespace libp2p::network::detail {

  // suppresses "no symbols" warning from linker
  const char *__library_name = "default network implementation";

}  // namespace libp2p::network::detail
