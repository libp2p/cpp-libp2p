/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cassert>
#include <libp2p/protocol/kademlia/scheduler.hpp>

namespace libp2p::protocol::kademlia {

  Scheduler::Scheduler() : counter_(0) {}

  Scheduler::Handle Scheduler::schedule(Scheduler::Ticks delay,
                                        Scheduler::Callback cb) {
    assert(cb);
    return Handle(newTicket(delay, std::move(cb)), weak_from_this());
  }

  Scheduler::Ticket Scheduler::newTicket(Scheduler::Ticks delay,
                                         Scheduler::Callback cb) {
    auto abs_delay = (delay != 0) ? now() + delay : 0;
    Ticket ticket(abs_delay, ++counter_);
    table_[ticket] = std::move(cb);
    if (delay == 0) {
      scheduleImmediate();
    }
    return ticket;
  }

  Scheduler::Handle Scheduler::schedule(Scheduler::Callback cb) {
    return schedule(0, std::move(cb));
  }

  void Scheduler::cancel(const Ticket &ticket) {
    table_.erase(ticket);
  }

  Scheduler::Ticket Scheduler::reschedule(const Ticket &ticket, Ticks delay) {
    assert(ticket.second);
    if (ticket.second == counter_in_progress_) {
      // recheduling from inside current callback
      return newTicket(delay, cb_in_progress_);
    }
    auto it = table_.find(ticket);
    assert (it != table_.end());
    auto cb = std::move(it->second);
    table_.erase(it);
    return newTicket(delay, std::move(cb));
  }

  void Scheduler::pulse(bool immediate) {
    Ticks time = immediate ? 0 : now();
    while (nextCallback(time)) {
      cb_in_progress_();
    }
    counter_in_progress_ = 0;
    cb_in_progress_ = Callback();
  }

  bool Scheduler::nextCallback(Ticks time) {
    if (table_.empty()) {
      return false;
    }
    auto it = table_.begin();
    if (it->first.first > time) {
      return false;
    }
    counter_in_progress_ = it->first.second;
    cb_in_progress_ = std::move(it->second);
    table_.erase(it);
    return true;
  }

}  // namespace libp2p::protocol::kademlia
