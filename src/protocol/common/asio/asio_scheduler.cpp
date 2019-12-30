/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/common/asio/asio_scheduler.hpp>

namespace libp2p::protocol {

  AsioScheduler::AsioScheduler(boost::asio::io_context &io,
                               Scheduler::Ticks interval)
      : io_(io),
        timer_(io, boost::posix_time::milliseconds(interval)),
        interval_(interval),
        started_(boost::posix_time::microsec_clock::local_time()),
        timer_cb_([this](const boost::system::error_code &) { onTimer(); }),
        immediate_cb_([this] { onImmediate(); }) {
    timer_.async_wait(timer_cb_);
  }

  Scheduler::Ticks AsioScheduler::now() const {
    boost::posix_time::ptime t(boost::posix_time::microsec_clock::local_time());
    return (t - started_).total_milliseconds();
  }

  void AsioScheduler::scheduleImmediate() {
    if (!immediate_cb_scheduled_) {
      immediate_cb_scheduled_ = true;
      io_.post(immediate_cb_);
    }
  }

  void AsioScheduler::onTimer() {
    pulse(false);
    timer_.expires_at(timer_.expires_at()
                      + boost::posix_time::milliseconds(interval_));
    timer_.async_wait(timer_cb_);
  }

  void AsioScheduler::onImmediate() {
    if (immediate_cb_scheduled_) {
      immediate_cb_scheduled_ = false;
      pulse(true);
    }
  }

}  // namespace libp2p::protocol
