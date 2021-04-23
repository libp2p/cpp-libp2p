/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_ASIO_SCHEDULER_IMPL_HPP
#define LIBP2P_PROTOCOL_ASIO_SCHEDULER_IMPL_HPP

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <libp2p/protocol/common/scheduler.hpp>

namespace libp2p::protocol {

  class AsioScheduler : public Scheduler {
   public:
    AsioScheduler(std::shared_ptr<boost::asio::io_context> io,
                  SchedulerConfig config);

    ~AsioScheduler() override;

   private:
    Ticks now() const override;

    void scheduleImmediate() override;

    void onTimer();

    void onImmediate();

    std::shared_ptr<boost::asio::io_context> io_;
    Ticks interval_;
    boost::asio::deadline_timer timer_;
    boost::posix_time::ptime started_;

    /*
     * Timer callback is canceled
     *
     * In case when the timer has already expired when cancel() is called and
     * the handlers have been invoked or queued for invocation. These handlers
     * can no longer be cancelled, and therefore are passed an error code that
     * indicates the successful completion of the wait operation. Cancel is used
     * to indicate that handler should not be executed.
     */
    std::shared_ptr<bool> canceled_;
    std::function<void(const boost::system::error_code &)> timer_cb_;
    std::function<void()> immediate_cb_;
    bool immediate_cb_scheduled_ = false;
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_ASIO_SCHEDULER_IMPL_HPP
