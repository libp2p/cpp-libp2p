/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>

namespace libp2p::basic {

  void ManualSchedulerBackend::setTimer(
      std::chrono::milliseconds abs_time,
      std::weak_ptr<SchedulerBackendFeedback> scheduler) {
    if (abs_time == std::chrono::milliseconds::zero()) {
      deferred_callbacks_.emplace_back([scheduler = std::move(scheduler)]() {
        auto sch = scheduler.lock();
        if (sch) {
          sch->pulse(kZeroTime);
        }
      });
      return;
    }

    assert(abs_time.count() > 0);

    timer_expires_ = abs_time;

    timer_callback_ = [this, scheduler = std::move(scheduler)]() {
      auto sch = scheduler.lock();
      if (sch) {
        sch->pulse(now());
      }
    };
  }

  void ManualSchedulerBackend::shift(std::chrono::milliseconds delta) {
    if (delta.count() < 0) {
      delta = std::chrono::milliseconds::zero();
    }
    current_clock_ += delta;

    if (!deferred_callbacks_.empty()) {
      in_process_.swap(deferred_callbacks_);
      deferred_callbacks_.clear();
      for (const auto &cb : in_process_) {
        cb();
      }
      in_process_.clear();
    }

    if (timer_callback_ && current_clock_ >= timer_expires_) {
      Callback cb;
      cb.swap(timer_callback_);
      timer_expires_ = std::chrono::milliseconds::zero();
      cb();
    }
  }

  void ManualSchedulerBackend::shiftToTimer() {
    auto delta = std::chrono::milliseconds::zero();
    if (timer_expires_ > current_clock_) {
      delta = timer_expires_ - current_clock_;
    }
    shift(delta);
  }

}  // namespace libp2p::basic
