/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <memory>

#include <libp2p/common/logger.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/network/cares/cares.hpp>
#include <libp2p/outcome/outcome.hpp>

int main(int argc, char *argv[]) {
  auto ares = std::make_shared<libp2p::network::cares::Ares>();
  spdlog::set_level(spdlog::level::trace);

  // create a default Host via an injector
  auto injector = libp2p::injector::makeHostInjector();

  // create io_context - in fact, thing, which allows us to execute async
  // operations
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  // the guard to preserve context's running state when tasks queue is empty
  auto work_guard = boost::asio::make_work_guard(*context);
  context->post([&] {
    ares->resolveTxt(
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
          // normal conditions for libp2p imply the main context is running and
          // serving listeners or something at the moment
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
