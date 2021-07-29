/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_SCHEDULER_HPP
#define LIBP2P_BASIC_SCHEDULER_HPP

#include <chrono>
#include <functional>
#include <memory>

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::basic {

  constexpr std::chrono::milliseconds kZeroTime =
      std::chrono::milliseconds::zero();

  /**
   * Feedback from scheduler backend to Scheduler implementation.
   */
  class SchedulerBackendFeedback {
   public:
    virtual ~SchedulerBackendFeedback() = default;

    /**
     * Called from backend to fire callbacks
     * @param current_clock
     * For timers: non-zero current async ticks in milliseconds since async's
     * epoch;
     * For callbacks deferred to next IO loop cycle: zero
     */
    virtual void pulse(std::chrono::milliseconds current_clock) noexcept = 0;
  };

  /**
   * Scheduling engine that depends on implementation:
   * 1) AsioSchedulerBackend (for asio-based asynchrony, it uses
   * std::chrono::steady_clock)
   * 2) ManualSchedulerBackend (for testing purposes,
   * uses manual time shifts as a async)
   */
  class SchedulerBackend {
   public:
    virtual ~SchedulerBackend() = default;

    /**
     * Current async
     * @return Milliseconds elapsed from async's epoch
     */
    virtual std::chrono::milliseconds now() const noexcept = 0;

    /**
     * Implementation-defined defer or delay function.
     * @param abs_time Time since async's epoch.
     * If abs_time == 0 then SchedulerBackendFeedback::pulse()
     * will be called on the next IO loop cycle with zero argument
     * @param scheduler Weak reference to scheduler (as it may expire)
     */
    virtual void setTimer(
        std::chrono::milliseconds abs_time,
        std::weak_ptr<SchedulerBackendFeedback> scheduler) = 0;
  };

  /**
   * Scheduler API. Provides callback deferring facilities and low-res timer
   */
  class Scheduler : public SchedulerBackendFeedback,
                    public std::enable_shared_from_this<Scheduler> {
   public:
    struct Config {
      static constexpr std::chrono::milliseconds kMaxTimerThreshold =
          std::chrono::milliseconds(10);

      /**
       * This threshold is needed to avoid too often timer switches.
       * Performance concerns
       */
      std::chrono::milliseconds max_timer_threshold = kMaxTimerThreshold;
    };

    enum class Error {
      // Invalid argument passed
      kInvalidArgument = 1,

      // Scheduler handle detached, cannot reschedule
      kHandleDetached,

      // Scheduler item not found, cannot reschedule
      kItemNotFound,
    };

    /**
     * Handle provides scoped callbacks lifetime, allows for canceling and
     * rescheduling
     */
    class Handle {
     public:
      /**
       * { async-time, seq-number } structure for proper ordering and uniqueness
       * within Scheduler
       */
      using Ticket = std::pair<std::chrono::milliseconds, uint64_t>;

      Handle(const Handle &) = delete;
      Handle &operator=(const Handle &) = delete;

      Handle() = default;
      Handle(Handle &&) = default;

      /**
       * Ctor. Called from SchedulerImpl
       * @param ticket unique ticket, used for cancelling and rescheduling
       * @param scheduler handle's owner object
       */
      Handle(Ticket ticket, std::weak_ptr<Scheduler> scheduler);

      /**
       *  Move assignment cancels existing ticket, if any
       */
      Handle &operator=(Handle &&r) noexcept;

      /**
       *  Non-trivial dtor cancels existing ticket, if any
       */
      ~Handle();

      /**
       * Cancels existing ticket, if any
       */
      void cancel() noexcept;

      /**
       * Reschedules existing ticket, if it is still active.
       * Allows reentrancy (can be called from inside the callback)
       * @param delay_from_now Relative time
       * @return success or error value from Scheduler::Error
       */
      outcome::result<void> reschedule(
          std::chrono::milliseconds delay_from_now) noexcept;

     private:
      Ticket ticket_;
      std::weak_ptr<Scheduler> scheduler_;
    };

    using Callback = std::function<void()>;

    /**
     * Defers callback to be executed during the next IO loop cycle
     * @param cb callback
     */
    void schedule(Callback &&cb) noexcept {
      std::ignore = scheduleImpl(std::move(cb), kZeroTime, false);
    }

    /**
     * Schedules callback to be executed after interval given
     * @param cb callback
     * @param delay_from_now time interval
     */
    void schedule(Callback &&cb,
                  std::chrono::milliseconds delay_from_now) noexcept {
      std::ignore = scheduleImpl(std::move(cb), delay_from_now, false);
    }

    /**
     * Defers callback to be executed during the next IO loop cycle
     * @param cb callback
     * @return handle which can be used for cancelling, rescheduling, and scoped
     * lifetime
     */
    [[nodiscard]] Handle scheduleWithHandle(Callback &&cb) noexcept {
      return scheduleImpl(std::move(cb), kZeroTime, true);
    }

    /**
     * Schedules callback to be executed after interval given
     * @param cb callback
     * @param delay_from_now time interval
     * @return handle which can be used for cancelling, rescheduling, and scoped
     * lifetime
     */
    [[nodiscard]] Handle scheduleWithHandle(
        Callback &&cb, std::chrono::milliseconds delay_from_now) noexcept {
      return scheduleImpl(std::move(cb), delay_from_now, true);
    }

    /**
     * Backend's async
     * @return milliseconds since async's epoch
     */
    virtual std::chrono::milliseconds now() const noexcept = 0;

    /**
     * Doesn't allow lvalue callbacks
     */
    [[maybe_unused]] Handle scheduleImpl(Callback &cb,
                                         std::chrono::milliseconds,
                                         bool) = delete;

   protected:
    /// Handle calls cancel() and reschedule()
    friend class Handle;

    /**
     * Called from schedule() and scheduleWithHandle() functions
     * @param cb callback
     * @param delay_from_now time interval, zero for deferring
     * @param make_handle if true, then active Handle is returned
     * @return actie or empty Handle, depending on make_handle argument
     */
    virtual Handle scheduleImpl(Callback &&cb,
                                std::chrono::milliseconds delay_from_now,
                                bool make_handle) noexcept = 0;

    /**
     * Called from Handle (from move assignment, destructor, or manually)
     * @param ticket handle's existing ticket
     */
    virtual void cancel(Handle::Ticket ticket) noexcept = 0;

    /**
     * Called from Handle.
     * Reschedules existing ticket.
     * Allows reentrancy (can be called from inside the callback)
     * @param delay_from_now Relative time
     * @return success or error value from Scheduler::Error
     */
    virtual outcome::result<Handle::Ticket> reschedule(
        Handle::Ticket ticket,
        std::chrono::milliseconds delay_from_now) noexcept = 0;
  };
}  // namespace libp2p::basic

OUTCOME_HPP_DECLARE_ERROR(libp2p::basic, Scheduler::Error)

#endif  // LIBP2P_BASIC_SCHEDULER_HPP
