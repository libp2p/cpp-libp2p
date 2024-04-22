/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>

namespace libp2p::basic {
  void ManualSchedulerBackend::post(std::function<void()> &&cb) {
    deferred_callbacks_.emplace_back(cb);
  }

  void ManualSchedulerBackend::setTimer(
      std::chrono::milliseconds abs_time,
      std::weak_ptr<SchedulerBackendFeedback> scheduler) {
    timer_expires_ = abs_time;
    scheduler_ = scheduler;
  }

  void ManualSchedulerBackend::shift(std::chrono::milliseconds delta) {
    callDeferred();
    current_clock_ += std::max(delta, std::chrono::milliseconds::zero());
    if (timer_expires_ and *timer_expires_ <= current_clock_) {
      timer_expires_.reset();
      if (auto scheduler = scheduler_.lock()) {
        scheduler->pulse();
      }
    }
    callDeferred();
  }

  void ManualSchedulerBackend::shiftToTimer() {
    shift(std::max(timer_expires_.value_or(current_clock_), current_clock_)
          - current_clock_);
  }

  void ManualSchedulerBackend::callDeferred() {
    while (not deferred_callbacks_.empty()) {
      auto cb = std::move(deferred_callbacks_.front());
      deferred_callbacks_.pop_front();
      cb();
    }
  }
}  // namespace libp2p::basic
