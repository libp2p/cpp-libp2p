/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/log/logger.hpp>
#include <mutex>
#include <soralog/injector.hpp>

#ifndef TESTUTIL_PREPARELOGGERS
#define TESTUTIL_PREPARELOGGERS

namespace testutil {

  static std::once_flag initialized;

  void prepareLoggers(soralog::Level level = soralog::Level::INFO) {
    std::call_once(initialized, [] {
      auto injector = soralog::injector::makeInjector();

      auto logger_system =
          injector.create<std::shared_ptr<soralog::LoggerSystem>>();

      auto r = logger_system->configure();
      if (r.has_error) {
        throw std::runtime_error("Can't configure logger system");
      }

      libp2p::log::setLoggerSystem(logger_system);
    });

    libp2p::log::setLevelOfGroup("*", level);
  }

}  // namespace testutil

#endif  // TESTUTIL_PREPARELOGGERS
