/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/scheduler.hpp>
#include <cassert>

namespace libp2p::protocol::kademlia {

  Scheduler::Scheduler() : last_tick_(0), counter_(0) {}

  Scheduler::~Scheduler() {
    for (auto &p : table_) {
      p.second->done();
    }
  }

  Scheduler::Handle Scheduler::schedule(Scheduler::Ticks delay, Scheduler::Callback cb) {
    assert(cb);
    auto abs_delay = (delay != 0) ? now() + delay : 0;
    Ticket ticket(abs_delay, ++counter_);
    Handle h(new scheduler::Handle(ticket, this, std::move(cb)));
    table_[ticket] = h.get();
    //    log_->info("Timer create: now={}, table size={}", last_tick_, table_.size());
    if (delay == 0) {
      scheduleImmediate();
    }
    return h;
  }

  Scheduler::Handle Scheduler::schedule(Scheduler::Callback cb) {
    return schedule(0, std::move(cb));
  }

  void Scheduler::cancel(const Ticket &ticket) {
    table_.erase(ticket);
    //     log_->info("Timer cancel: now={}, table size={}", last_tick_, table_.size());
  }

  void Scheduler::pulse(bool immediate) {
    if (!immediate) {
      last_tick_ = now();
    }
    for (;;) {
      //   log_->info("Timer pulse: now={}, table size={}", last_tick_, table_.size());
      scheduler::Handle *h = getHandle(immediate);
      if (!h) {
        break;
      }
      h->call();
    }
  }

  scheduler::Handle *Scheduler::getHandle(bool immediate) {
    if (table_.empty()) {
      return nullptr;
    }
    auto it = table_.begin();
    auto time = it->first.first;
    if (immediate) {
      if (time != 0) {
        return nullptr;
      }
    } else if (time > last_tick_) {
      return nullptr;
    }
    scheduler::Handle *h = it->second;
    table_.erase(it);
    return h;
  }

} //namespace
