/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/log/logger.hpp>

#include <mutex>
#include <soralog/impl/fallback_configurator.hpp>

#ifndef TESTUTIL_PREPARELOGGERS
#define TESTUTIL_PREPARELOGGERS

namespace testutil {

  static std::once_flag initialized;

  void prepareLoggers(soralog::Level level = soralog::Level::INFO) {
    std::call_once(initialized, [] {
      auto configurator = std::make_shared<soralog::FallbackConfigurator>();

      auto logging_system =
          std::make_shared<soralog::LoggingSystem>(configurator);

      auto r = logging_system->configure();
      if (r.has_error) {
        throw std::runtime_error("Can't configure logger system");
      }

      libp2p::log::setLoggingSystem(logging_system);
    });

    libp2p::log::setLevelOfGroup("*", level);
  }

}  // namespace testutil

#endif  // TESTUTIL_PREPARELOGGERS
