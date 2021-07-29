/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MANUAL_SCHEDULER_BACKEND_HPP
#define LIBP2P_MANUAL_SCHEDULER_BACKEND_HPP

#include <vector>

#include <libp2p/basic/scheduler.hpp>

namespace libp2p::basic {

  /**
   * Manual scheduler backend implementation, uses manual time shifts and
   * internal pseudo timer. Injected into SchedulerImpl.
   */
  class ManualSchedulerBackend : public SchedulerBackend {
    using Callback = std::function<void()>;

   public:
    ManualSchedulerBackend() : current_clock_(1) {}

    /**
     * @return Milliseconds since clock's epoch. Clock is set manually
     */
    std::chrono::milliseconds now() const noexcept override {
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
      return deferred_callbacks_.empty() && !timer_callback_;
    }

   private:
    /// Current time, set manually
    std::chrono::milliseconds current_clock_;

    /// Callbacks deferred for the next cycle
    std::vector<Callback> deferred_callbacks_;

    /// Currently processed callback (reentrancy + rescheduling reasons here)
    std::vector<Callback> in_process_;

    /// Timer callback
    Callback timer_callback_;

    /// Expiry of timer event
    std::chrono::milliseconds timer_expires_{};
  };

}  // namespace libp2p::basic

#endif  // LIBP2P_BASIC_ASIO_SCHEDULER_BACKEND_HPP
