/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/scheduler/scheduler_impl.hpp>

namespace libp2p::basic {

  SchedulerImpl::SchedulerImpl(std::shared_ptr<SchedulerBackend> backend,
                               Scheduler::Config config)
      : backend_{std::move(backend)}, config_{config} {}

  std::chrono::milliseconds SchedulerImpl::now() const {
    return backend_->now();
  }

  Scheduler::Handle SchedulerImpl::scheduleImpl(
      Callback &&cb,
      std::chrono::milliseconds delay_from_now,
      bool make_handle) {
    if (not cb) {
      throw std::logic_error{"SchedulerImpl::scheduleImpl empty cb arg"};
    }

    auto abs = Time::zero();
    if (Time::zero() < delay_from_now) {
      abs = backend_->now() + delay_from_now;
    }
    if (not make_handle) {
      backend_->post(
          [weak_self{weak_from_this()}, cb{std::move(cb)}, abs]() mutable {
            auto self = weak_self.lock();
            if (not self) {
              return;
            }
            self->callbacks_.emplace(abs, std::move(cb));
            self->pulse();
          });
      return Cancel{};
    }
    auto cancel = std::make_shared<CancelCb>(std::move(cb));
    std::weak_ptr weak_cancel{cancel};
    backend_->post(
        [weak_self{weak_from_this()}, abs, cancel{std::move(cancel)}] {
          auto self = weak_self.lock();
          if (not self) {
            return;
          }
          if (cancel->cancelled.test()) {
            return;
          }
          cancel->it = self->callbacks_.emplace(abs, cancel);
          self->pulse();
        });
    return cancelFn(
        [weak_self{weak_from_this()}, weak_cancel{std::move(weak_cancel)}] {
          auto cancel = weak_cancel.lock();
          if (not cancel) {
            return;
          }
          if (cancel->cancelled.test_and_set()) {
            return;
          }
          auto self = weak_self.lock();
          if (not self) {
            return;
          }
          self->backend_->post([weak_self, weak_cancel] {
            auto cancel = weak_cancel.lock();
            if (not cancel) {
              return;
            }
            if (not cancel->it) {
              return;
            }
            auto self = weak_self.lock();
            if (not self) {
              return;
            }
            self->callbacks_.erase(*cancel->it);
          });
        });
  }

  void SchedulerImpl::pulse() {
    callReady(Time::zero());
    while (not callbacks_.empty()) {
      auto now = backend_->now();
      if (callReady(now) != 0) {
        continue;
      }
      auto min = callbacks_.begin()->first;
      if (now < timer_ and timer_ <= min + config_.max_timer_threshold) {
        return;
      }
      timer_ = std::max(now + config_.max_timer_threshold, min);
      backend_->setTimer(timer_, weak_from_this());
      return;
    }
  }

  size_t SchedulerImpl::callReady(Time now) {
    size_t removed = 0;
    while (not callbacks_.empty() and callbacks_.begin()->first <= now) {
      auto node = callbacks_.extract(callbacks_.begin());
      ++removed;
      if (auto cb = std::get_if<Callback>(&node.mapped())) {
        (*cb)();
      } else {
        auto &cancel = std::get<std::shared_ptr<CancelCb>>(node.mapped());
        if (cancel->cancelled.test_and_set()) {
          continue;
        }
        cancel->cb();
      }
    }
    return removed;
  }
}  // namespace libp2p::basic
