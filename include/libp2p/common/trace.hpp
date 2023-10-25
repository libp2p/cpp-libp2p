/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/log/logger.hpp>

#if TRACE_ENABLED

#define TRACE(FMT, ...)                            \
  do {                                             \
    auto log = libp2p::log::createLogger("debug"); \
    SL_TRACE(log, (FMT), ##__VA_ARGS__);           \
  } while (false)
#else
#define TRACE(...)
#endif
