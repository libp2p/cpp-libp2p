/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <functional>
#include <memory>

namespace libp2p::basic {
  /**
   * Feedback from scheduler backend to Scheduler implementation.
   */
  class SchedulerBackendFeedback {
   public:
    virtual ~SchedulerBackendFeedback() = default;

    /**
     * Called from backend to fire callbacks
     */
    virtual void pulse() = 0;
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
     * boost::asio::io_context::post
     */
    virtual void post(std::function<void()> &&) = 0;

    /**
     * Current async
     * @return Milliseconds elapsed from async's epoch
     */
    virtual std::chrono::milliseconds now() const = 0;

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
}  // namespace libp2p::basic
