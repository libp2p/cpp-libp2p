/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>

void libp2p_soralog() {
  std::string yaml = R"(
sinks:
 - name: console
   type: console
   color: true
groups:
 - name: main
   sink: console
   level: error
   children:
     - name: libp2p
)";
  auto log = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<soralog::ConfiguratorFromYAML>(
          std::make_shared<libp2p::log::Configurator>(), yaml));
  auto r = log->configure();
  if (not r.message.empty()) {
    fmt::println("{} {}", r.has_error ? "E" : "W", r.message);
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }
  libp2p::log::setLoggingSystem(log);
}
