/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <memory>

#include <libp2p/injector/host_injector.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/network/cares/cares.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace {
  const std::string logger_config(R"(
# ----------------
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
# ----------------
  )");
}  // namespace

int main(int argc, char *argv[]) {
  // prepare log system
  auto logging_system = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<soralog::ConfiguratorFromYAML>(
          // Original LibP2P logging config
          std::make_shared<libp2p::log::Configurator>(),
          // Additional logging config for application
          logger_config));
  auto r = logging_system->configure();
  if (not r.message.empty()) {
    (r.has_error ? std::cerr : std::cout) << r.message << std::endl;
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }

  libp2p::log::setLoggingSystem(logging_system);
  if (std::getenv("TRACE_DEBUG") != nullptr) {
    libp2p::log::setLevelOfGroup("main", soralog::Level::TRACE);
  } else {
    libp2p::log::setLevelOfGroup("main", soralog::Level::ERROR);
  }

  // create a default Host via an injector
  auto injector = libp2p::injector::makeHostInjector();

  libp2p::network::c_ares::Ares ares;

  // create io_context - in fact, thing, which allows us to execute async
  // operations
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  // the guard to preserve context's running state when tasks queue is empty
  auto work_guard = boost::asio::make_work_guard(*context);
  context->post([&] {
    ares.resolveTxt(
        "_dnsaddr.bootstrap.libp2p.io", context,
        [&](libp2p::outcome::result<std::vector<std::string>> result) -> void {
          if (result.has_error()) {
            std::cout << result.error().message() << std::endl;
          } else {
            auto &&val = result.value();
            for (auto &record : val) {
              std::cout << record << std::endl;
            }
          }
          // work guard is used only for example purposes.
          // normal conditions for libp2p imply the main context is running
          // and serving listeners or something at the moment
          work_guard.reset();
        });
  });
  // run the IO context
  try {
    context->run();
  } catch (const boost::system::error_code &ec) {
    std::cout << "Example cannot run: " << ec.message() << std::endl;
  } catch (...) {
    std::cout << "Unknown error happened" << std::endl;
  }
}
