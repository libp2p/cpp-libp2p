/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <libp2p/basic/cancel.hpp>

namespace libp2p::basic {
  /**
   * Scheduler API. Provides callback deferring facilities and low-res timer
   */
  class Scheduler {
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

    using Handle = Cancel;

    using Callback = std::function<void()>;
    using Time = std::chrono::milliseconds;

    virtual ~Scheduler() = default;

    /**
     * Defers callback to be executed during the next IO loop cycle
     * @param cb callback
     */
    void schedule(Callback &&cb) {
      std::ignore = scheduleImpl(std::move(cb), Time::zero(), false);
    }

    /**
     * Schedules callback to be executed after interval given
     * @param cb callback
     * @param delay_from_now time interval
     */
    void schedule(Callback &&cb, std::chrono::milliseconds delay_from_now) {
      std::ignore = scheduleImpl(std::move(cb), delay_from_now, false);
    }

    /**
     * Defers callback to be executed during the next IO loop cycle
     * @param cb callback
     * @return handle which can be used for cancelling, rescheduling, and scoped
     * lifetime
     */
    [[nodiscard]] Handle scheduleWithHandle(Callback &&cb) {
      return scheduleImpl(std::move(cb), Time::zero(), true);
    }

    /**
     * Schedules callback to be executed after interval given
     * @param cb callback
     * @param delay_from_now time interval
     * @return handle which can be used for cancelling, rescheduling, and scoped
     * lifetime
     */
    [[nodiscard]] Handle scheduleWithHandle(
        Callback &&cb, std::chrono::milliseconds delay_from_now) {
      return scheduleImpl(std::move(cb), delay_from_now, true);
    }

    /**
     * Backend's async
     * @return milliseconds since async's epoch
     */
    virtual std::chrono::milliseconds now() const = 0;

    /**
     * Doesn't allow lvalue callbacks
     */
    [[maybe_unused]] Handle scheduleImpl(Callback &cb,
                                         std::chrono::milliseconds,
                                         bool) = delete;

   protected:
    /**
     * Called from schedule() and scheduleWithHandle() functions
     * @param cb callback
     * @param delay_from_now time interval, zero for deferring
     * @param make_handle if true, then active Handle is returned
     * @return actie or empty Handle, depending on make_handle argument
     */
    virtual Handle scheduleImpl(Callback &&cb,
                                std::chrono::milliseconds delay_from_now,
                                bool make_handle) = 0;
  };
}  // namespace libp2p::basic
