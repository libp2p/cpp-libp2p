/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>

#include <libp2p/protocol/common/asio/asio_scheduler.hpp>

namespace libp2p::protocol {

  AsioScheduler::AsioScheduler(boost::asio::io_context &io,
                               SchedulerConfig config)
      : io_(io),
        interval_(config.period_msec),
        timer_(io, boost::posix_time::milliseconds(interval_)),
        started_(boost::posix_time::microsec_clock::local_time()),
        cancelled_(std::make_shared<bool>(false)),
        timer_cb_([this, cancelled{cancelled_}](
                      const boost::system::error_code &error) {
          if (!error && !*cancelled)
            onTimer();
        }),
        immediate_cb_([this] { onImmediate(); }) {
    assert(interval_ > 0 && interval_ <= 1000);
    timer_.async_wait(timer_cb_);
  }

  AsioScheduler::~AsioScheduler() {
    *cancelled_ = true;
    timer_.cancel();
  };

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
