/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTUTIL_PREPARELOGGERS
#define TESTUTIL_PREPARELOGGERS

#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>

#include <mutex>
#include <soralog/impl/configurator_from_yaml.hpp>

namespace testutil {

  static std::once_flag initialized;

  void prepareLoggers(soralog::Level level = soralog::Level::INFO) {
    std::call_once(initialized, [] {

      auto configurator = std::make_shared<soralog::ConfiguratorFromYAML>(std::string(R"(
sinks:
  - name: console
    type: console
groups:
  - name: libp2p
    sink: console
    level: off
)"));

      auto logging_system =
          std::make_shared<soralog::LoggingSystem>(configurator);

      auto r = logging_system->configure();
      if (r.has_error) {
        std::cerr << r.message << std::endl;
        exit(EXIT_FAILURE);
      }

      libp2p::log::setLoggingSystem(logging_system);
    });

    libp2p::log::setLevelOfGroup("*", level);
  }

}  // namespace testutil

#endif  // TESTUTIL_PREPARELOGGERS
