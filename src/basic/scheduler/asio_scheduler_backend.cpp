/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>

#include <boost/asio/post.hpp>

#include <libp2p/log/logger.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::basic {
  AsioSchedulerBackend::AsioSchedulerBackend(
      std::shared_ptr<boost::asio::io_context> io_context)
      : io_context_(std::move(io_context)), timer_(*io_context_) {}

  void AsioSchedulerBackend::post(std::function<void()> &&cb) {
    boost::asio::post(*io_context_, std::move(cb));
  }

  std::chrono::milliseconds AsioSchedulerBackend::now() const {
    return nowImpl();
  }

  void AsioSchedulerBackend::setTimer(
      std::chrono::milliseconds abs_time,
      std::weak_ptr<SchedulerBackendFeedback> scheduler) {
    timer_.expires_at(decltype(timer_)::clock_type::time_point(abs_time));
    timer_.async_wait([scheduler = std::move(scheduler)](
                          const boost::system::error_code &error) {
      if (!error) {
        auto sch = scheduler.lock();
        if (sch) {
          sch->pulse();
        }
      }
    });
  }

  std::chrono::milliseconds AsioSchedulerBackend::nowImpl() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        decltype(timer_)::clock_type::now().time_since_epoch());
  }

}  // namespace libp2p::basic
