/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/basic/scheduler.hpp>

namespace libp2p::basic {

  Scheduler::Handle::Handle(Scheduler::Handle::Ticket ticket,
                            std::weak_ptr<Scheduler> scheduler)
      : ticket_(std::move(ticket)), scheduler_(std::move(scheduler)) {}

  Scheduler::Handle &Scheduler::Handle::operator=(
      Scheduler::Handle &&r) noexcept {
    cancel();
    ticket_ = std::move(r.ticket_);
    scheduler_ = std::move(r.scheduler_);
    return *this;
  }

  Scheduler::Handle::~Handle() {
    cancel();
  }

  void Scheduler::Handle::cancel() noexcept {
    auto sch = scheduler_.lock();
    if (sch) {
      sch->cancel(ticket_);
      scheduler_.reset();
    }
  }

  outcome::result<void> Scheduler::Handle::reschedule(
      std::chrono::milliseconds delay_from_now) noexcept {
    if (delay_from_now.count() <= 0) {
      return Scheduler::Error::kInvalidArgument;
    }
    auto sch = scheduler_.lock();
    if (sch) {
      auto res = sch->reschedule(ticket_, delay_from_now);
      if (!res) {
        scheduler_.reset();
        return res.error();
      }
      ticket_ = std::move(res.value());
      return outcome::success();
    }
    return Scheduler::Error::kHandleDetached;
  }

}  // namespace libp2p::basic

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::basic, Scheduler::Error, e) {
  using E = libp2p::basic::Scheduler::Error;
  switch (e) {
    case E::kInvalidArgument:
      return "Scheduler: invalid argument";
    case E::kHandleDetached:
      return "Scheduler: handle detached, cannot reschedule";
    case E::kItemNotFound:
      return "Scheduler: item not found, cannot reschedule";
  }
  return "Scheduler: unknown error";
}
