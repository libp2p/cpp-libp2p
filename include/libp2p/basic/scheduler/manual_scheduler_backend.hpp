/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <deque>
#include <optional>

#include <libp2p/basic/scheduler/backend.hpp>

namespace libp2p::basic {

  /**
   * Manual scheduler backend implementation, uses manual time shifts and
   * internal pseudo timer. Injected into SchedulerImpl.
   */
  class ManualSchedulerBackend : public SchedulerBackend {
    using Callback = std::function<void()>;

   public:
    ManualSchedulerBackend() : current_clock_(1) {}

    void post(std::function<void()> &&) override;

    /**
     * @return Milliseconds since clock's epoch. Clock is set manually
     */
    std::chrono::milliseconds now() const override {
      return current_clock_;
    }

    /**
     * Sets the timer. Called by Scheduler implementation
     * @param abs_time Abs time: milliseconds since clock's epoch
     * @param scheduler Weak ref to owner
     */
    void setTimer(std::chrono::milliseconds abs_time,
                  std::weak_ptr<SchedulerBackendFeedback> scheduler) override;

    /**
     * Shifts internal timer by delta milliseconds, executes everything (i.e.
     * deferred and timed events) in between
     * @param delta Manual time shift in milliseconds
     */
    void shift(std::chrono::milliseconds delta);

    /**
     * Shifts internal timer to the nearest timer event, executes everything
     * (i.e. deferred and timed events) in between
     */
    void shiftToTimer();

    /**
     * @return true if no more events scheduled
     */
    bool empty() const {
      return deferred_callbacks_.empty() and not timer_expires_;
    }

    void run() {
      while (not empty()) {
        shiftToTimer();
      }
    }

   private:
    void callDeferred();

    /// Current time, set manually
    std::chrono::milliseconds current_clock_;

    /// Callbacks deferred for the next cycle
    std::deque<Callback> deferred_callbacks_;

    /// Timer callback
    std::weak_ptr<SchedulerBackendFeedback> scheduler_;

    /// Expiry of timer event
    std::optional<std::chrono::milliseconds> timer_expires_;
  };

}  // namespace libp2p::basic
