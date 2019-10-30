/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/host/default_host.hpp>

namespace libp2p::host::detail {

  // suppresses "no symbols" warning from linker
  const char *__library_name = "default host library implementation";
}  // namespace libp2p::host::detail
