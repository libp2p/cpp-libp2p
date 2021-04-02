/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_COMMON_TRACE_HPP
#define LIBP2P_COMMON_TRACE_HPP

#include <libp2p/log/logger.hpp>

#if TRACE_ENABLED

#define TRACE(FMT, ...)                                            \
  do {                                                             \
    auto log = libp2p::log::createLogger("debug"); \
    SL_TRACE(log, (FMT), ##__VA_ARGS__);                           \
  } while (false)
#else
#define TRACE(...)
#endif

#endif  // LIBP2P_COMMON_TRACE_HPP
