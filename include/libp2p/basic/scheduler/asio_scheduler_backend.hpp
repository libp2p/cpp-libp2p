/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>

#include <libp2p/basic/scheduler/backend.hpp>

namespace libp2p::basic {

  /**
   * Boost.Asio scheduler backend implementation, uses steady timer.
   * Injected into SchedulerImpl. Has only 1 timer, which is rescheduled as per
   * higher level logic
   */
  class AsioSchedulerBackend : public SchedulerBackend {
   public:
    /**
     * Ctor.
     * @param io_context Asio io context
     */
    explicit AsioSchedulerBackend(
        std::shared_ptr<boost::asio::io_context> io_context);

    void post(std::function<void()> &&) override;

    /**
     * @return Milliseconds since steady clock's epoch
     */
    std::chrono::milliseconds now() const override;

    /**
     * Sets the timer. Called by Scheduler implementation
     * @param abs_time Abs time: milliseconds since clock's epoch
     * @param scheduler Weak ref to owner
     */
    void setTimer(std::chrono::milliseconds abs_time,
                  std::weak_ptr<SchedulerBackendFeedback> scheduler) override;

   private:
    /**
     * Non-virtual now() implementation
     */
    static std::chrono::milliseconds nowImpl();

    /// Boost.Asio context
    std::shared_ptr<boost::asio::io_context> io_context_;

    /// Boost.Asio steady timer
    boost::asio::steady_timer timer_;
  };

}  // namespace libp2p::basic
