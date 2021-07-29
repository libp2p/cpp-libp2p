/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>

#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

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
  // scheduler->schedule(fn);
}

TEST(Scheduler, BasicThings) {
  using namespace libp2p::basic;

  auto io = std::make_shared<boost::asio::io_context>(1);
  auto backend = std::make_shared<AsioSchedulerBackend>(io);
  auto scheduler =
      std::make_shared<SchedulerImpl>(std::move(backend), Scheduler::Config{});

  scheduler->schedule([]() { log().debug("deferred w/o handle called"); });

  scheduler->schedule([]() { log().debug("timed w/o handle called (155)"); },
                      std::chrono::milliseconds(155));

  auto h1 = scheduler->scheduleWithHandle(
      []() { log().debug("deferred w/handle called"); });

  auto h2 = scheduler->scheduleWithHandle(
      []() { log().debug("timed w/handle called (45)"); },
      std::chrono::milliseconds(45));

  Scheduler::Handle h3;
  h3 = scheduler->scheduleWithHandle(
      [first_time = true, &h3]() mutable {
        if (first_time) {
          log().debug(
              "timed w/handle called first time (98), rescheduling +70");
          first_time = false;
          EXPECT_OUTCOME_SUCCESS(_,
                                 h3.reschedule(std::chrono::milliseconds(70)));
        } else {
          log().debug("timed w/handle called second time, rescheduled");
        }
      },
      std::chrono::milliseconds(98));

  Scheduler::Handle h4;
  h4 = scheduler->scheduleWithHandle([first_time = true, &h4]() mutable {
    if (first_time) {
      log().debug("deferred w/handle called first time, rescheduling +120");
      first_time = false;
      EXPECT_OUTCOME_SUCCESS(_, h4.reschedule(std::chrono::milliseconds(120)));
    } else {
      log().debug("deferred->timed w/handle called second time, rescheduled");
    }
  });

  auto h5 = scheduler->scheduleWithHandle(
      []() { ASSERT_FALSE("h5 should not be called"); },
      std::chrono::milliseconds(78));

  auto h6 = scheduler->scheduleWithHandle(
      [&h5]() {
        h5.cancel();
        log().debug("h6 cancelled h5");
      },
      std::chrono::milliseconds(77));

  io->run_for(std::chrono::milliseconds(300));
}

TEST(Scheduler, ManualScheduler) {
  using namespace libp2p::basic;

  auto backend = std::make_shared<ManualSchedulerBackend>();
  auto scheduler =
      std::make_shared<SchedulerImpl>(backend, Scheduler::Config{});

  scheduler->schedule([]() { log().debug("deferred w/o handle called"); });

  scheduler->schedule([]() { log().debug("timed w/o handle called (155)"); },
                      std::chrono::milliseconds(155));

  auto h1 = scheduler->scheduleWithHandle(
      []() { log().debug("deferred w/handle called"); });

  auto h2 = scheduler->scheduleWithHandle(
      []() { log().debug("timed w/handle called (45)"); },
      std::chrono::milliseconds(45));

  Scheduler::Handle h3;
  h3 = scheduler->scheduleWithHandle(
      [first_time = true, &h3]() mutable {
        if (first_time) {
          log().debug(
              "timed w/handle called first time (98), rescheduling +70");
          first_time = false;
          EXPECT_OUTCOME_SUCCESS(_,
                                 h3.reschedule(std::chrono::milliseconds(70)));
        } else {
          log().debug("timed w/handle called second time, rescheduled");
        }
      },
      std::chrono::milliseconds(98));

  Scheduler::Handle h4;
  h4 = scheduler->scheduleWithHandle([first_time = true, &h4]() mutable {
    if (first_time) {
      log().debug("deferred w/handle called first time, rescheduling +120");
      first_time = false;
      EXPECT_OUTCOME_SUCCESS(_, h4.reschedule(std::chrono::milliseconds(120)));
    } else {
      log().debug("deferred->timed w/handle called second time, rescheduled");
    }
  });

  auto h5 = scheduler->scheduleWithHandle(
      []() { ASSERT_FALSE("h5 should not be called"); },
      std::chrono::milliseconds(78));

  auto h6 = scheduler->scheduleWithHandle(
      [&h5]() {
        h5.cancel();
        log().debug("h6 cancelled h5");
      },
      std::chrono::milliseconds(77));

  while (!backend->empty()) {
    backend->shift(std::chrono::milliseconds(1));
  }
}
