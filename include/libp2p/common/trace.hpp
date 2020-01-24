/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/common/logger.hpp>

#ifndef LIBP2P_COMMON_TRACE_HPP
#define LIBP2P_COMMON_TRACE_HPP

namespace libp2p::common {

  /// Special debug utility function, allows for not having logger as member
  /// field
  template <typename... Args>
  inline void traceToDebugLogger(spdlog::string_view_t fmt,
                                 const Args &... args) {
    static const std::string tag("debug");
    auto log = common::createLogger(tag);
    log->trace(fmt, args...);
  }

}  // namespace libp2p::common

#if TRACE_ENABLED
#define TRACE(...) libp2p::common::traceToDebugLogger(__VA_ARGS__)
#else
#define TRACE(...)
#endif

#endif  // LIBP2P_COMMON_TRACE_HPP
