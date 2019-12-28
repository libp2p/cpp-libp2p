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
    ~AsioScheduler() override = default;

    AsioScheduler(boost::asio::io_context &io, Ticks interval);

   private:
    Ticks now() const override;

    void scheduleImmediate() override;

    void onTimer();

    void onImmediate();

    boost::asio::io_context &io_;
    boost::asio::deadline_timer timer_;
    Ticks interval_;
    boost::posix_time::ptime started_;
    std::function<void(const boost::system::error_code &)> timer_cb_;
    std::function<void()> immediate_cb_;
    bool immediate_cb_scheduled_ = false;
  };

}  // namespace libp2p::protocol

#endif  // LIBP2P_PROTOCOL_ASIO_SCHEDULER_IMPL_HPP
