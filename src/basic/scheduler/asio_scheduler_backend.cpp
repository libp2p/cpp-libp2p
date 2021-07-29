/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>

#include <libp2p/log/logger.hpp>

namespace libp2p::basic {
  AsioSchedulerBackend::AsioSchedulerBackend(
      std::shared_ptr<boost::asio::io_context> io_context)
      : io_context_(std::move(io_context)), timer_(*io_context_) {}

  std::chrono::milliseconds AsioSchedulerBackend::now() const noexcept {
    return nowImpl();
  }

  void AsioSchedulerBackend::setTimer(
      std::chrono::milliseconds abs_time,
      std::weak_ptr<SchedulerBackendFeedback> scheduler) {
    if (abs_time == kZeroTime) {
      io_context_->post([scheduler = std::move(scheduler)]() {
        auto sch = scheduler.lock();
        if (sch) {
          sch->pulse(kZeroTime);
        }
      });
      return;
    }

    assert(abs_time.count() > 0);

    boost::system::error_code ec;
    timer_.expires_at(decltype(timer_)::clock_type::time_point(abs_time), ec);

    if (ec) {
      // this should never happen
      auto log = log::createLogger("Scheduler", "scheduler");
      log->critical("cannot set timer: {}", ec.message());
      boost::asio::detail::throw_error(ec, "setTimer");
    }

    timer_.async_wait([scheduler = std::move(scheduler)](
                          const boost::system::error_code &error) {
      if (!error) {
        auto sch = scheduler.lock();
        if (sch) {
          sch->pulse(nowImpl());
        }
      }
    });
  }

  std::chrono::milliseconds AsioSchedulerBackend::nowImpl() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        decltype(timer_)::clock_type::now().time_since_epoch());
  }

}  // namespace libp2p::basic
