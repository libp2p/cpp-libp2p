/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_ASIO_SCHEDULER_IMPL_HPP
#define LIBP2P_ASIO_SCHEDULER_IMPL_HPP

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <libp2p/protocol/kademlia/scheduler.hpp>

namespace libp2p::protocol::kademlia {

  class AsioScheduler : public Scheduler {
   public:
    static std::shared_ptr<AsioScheduler> create(boost::asio::io_context &io, Ticks interval) {
      return std::make_shared<AsioScheduler>(io, interval);
    }

    AsioScheduler(boost::asio::io_context &io, Ticks interval)
        : Scheduler{},
          io_(io),
          timer_(io, boost::posix_time::milliseconds(interval)),
          interval_(interval),
          started_(boost::posix_time::microsec_clock::local_time()),
          timer_cb_([this](const boost::system::error_code &) { onTimer(); }),
          immediate_cb_([this] { onImmediate(); })

    {
      timer_.async_wait(timer_cb_);
    }

   private:

    Ticks now() override {
      boost::posix_time::ptime t(
          boost::posix_time::microsec_clock::local_time());
      return (t - started_).total_milliseconds();
    }

    void scheduleImmediate() override {
      if (!immediate_cb_scheduled_) {
        immediate_cb_scheduled_ = true;
        io_.post(immediate_cb_);
      }
    }

    void onTimer() {
      pulse(false);
      timer_.expires_at(timer_.expires_at()
                        + boost::posix_time::milliseconds(interval_));
      timer_.async_wait(timer_cb_);
    }

    void onImmediate() {
      if (immediate_cb_scheduled_) {
        immediate_cb_scheduled_ = false;
        pulse(true);
      }
    }

    boost::asio::io_context &io_;
    boost::asio::deadline_timer timer_;
    Ticks interval_;
    boost::posix_time::ptime started_;
    std::function<void(const boost::system::error_code &)> timer_cb_;
    std::function<void()> immediate_cb_;
    bool immediate_cb_scheduled_ = false;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_ASIO_SCHEDULER_IMPL_HPP
