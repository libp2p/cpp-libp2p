/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/common/shared_fn.hpp>

#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using libp2p::SharedFn;
using libp2p::basic::Scheduler;

namespace {
  auto &log() {
    static auto logger = []() {
      if (std::getenv("TRACE_DEBUG") != nullptr) {
        testutil::prepareLoggers(soralog::Level::TRACE);
      } else {
        testutil::prepareLoggers(soralog::Level::INFO);
      }
      auto log = libp2p::log::createLogger("test");
      return log;
    }();
    return *logger;
  }
}  // namespace

// Contract checker, left here intentionally, though unused at the moment
void shouldNotCompile() {
  using namespace libp2p::basic;

  auto backend = std::make_shared<ManualSchedulerBackend>();
  auto scheduler =
      std::make_shared<SchedulerImpl>(backend, Scheduler::Config{});

  // Need to make wrapper to avoid lvalues also in std::function args

  auto fn = std::function<void()>(
      []() { log().debug("deferred w/o handle called"); });

  // N.B. this comment left intentionally, must not compile (lvalue)
  EXPECT_FALSE([&](auto &scheduler) {
    return requires { scheduler.schedule(fn); };
  }(*scheduler));
}

auto timers(std::shared_ptr<Scheduler> scheduler) {
  scheduler->schedule([]() { log().debug("deferred w/o handle called"); });

  scheduler->schedule([]() { log().debug("timed w/o handle called (155)"); },
                      std::chrono::milliseconds(155));

  auto h1 = scheduler->scheduleWithHandle(
      []() { log().debug("deferred w/handle called"); });

  auto h2 = scheduler->scheduleWithHandle(
      []() { log().debug("timed w/handle called (45)"); },
      std::chrono::milliseconds(45));

  auto h3 = scheduler->scheduleWithHandle(
      [] { log().debug("timed w/handle called (98)"); },
      std::chrono::milliseconds(98));

  auto h4 = scheduler->scheduleWithHandle(
      [] { log().debug("deferred w/handle called"); });

  auto h5 = scheduler->scheduleWithHandle(
      []() { ASSERT_FALSE("h5 should not be called"); },
      std::chrono::milliseconds(78));

  auto h6 =
      scheduler->scheduleWithHandle(SharedFn{[h5{std::move(h5)}]() mutable {
                                      h5.reset();
                                      log().debug("h6 cancelled h5");
                                    }},
                                    std::chrono::milliseconds(77));

  return std::tuple{
      std::move(h1),
      std::move(h2),
      std::move(h3),
      std::move(h4),
      std::move(h6),
  };
}

TEST(Scheduler, BasicThings) {
  using namespace libp2p::basic;

  auto io = std::make_shared<boost::asio::io_context>(1);
  auto backend = std::make_shared<AsioSchedulerBackend>(io);
  auto scheduler =
      std::make_shared<SchedulerImpl>(std::move(backend), Scheduler::Config{});

  auto h = timers(scheduler);

  io->run_for(std::chrono::milliseconds(300));
}

TEST(Scheduler, ManualScheduler) {
  using namespace libp2p::basic;

  auto backend = std::make_shared<ManualSchedulerBackend>();
  auto scheduler =
      std::make_shared<SchedulerImpl>(backend, Scheduler::Config{});

  auto h = timers(scheduler);

  backend->run();
}
