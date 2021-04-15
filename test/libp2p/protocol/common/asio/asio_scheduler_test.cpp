/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/common/asio/asio_scheduler.hpp"

#include <gtest/gtest.h>
#include <libp2p/injector/host_injector.hpp>

using libp2p::protocol::Scheduler;

/**
 * @given Constructs AsioScheduler and schedules on io context and then deletes
 * AsioScheduler
 * @when context is run
 * @then scheduler called and can handle cancellation timer without segfault
 */
TEST(AsioScheduler, Construct) {
  auto injector = libp2p::injector::makeHostInjector();
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  std::shared_ptr<Scheduler> scheduler =
      std::make_shared<libp2p::protocol::AsioScheduler>(
          context, libp2p::protocol::SchedulerConfig{});
  scheduler.reset();
  context->run();
}
