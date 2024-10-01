#pragma once

#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>

void log() {
  std::string yaml = R"(
sinks:
 - name: console
   type: console
   color: true
groups:
 - name: main
   sink: console
   level: info
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
