/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/common/subscription.hpp>

#include <cassert>

namespace libp2p::protocol {

  Subscription::Subscription() : ticket_(0) {}

  Subscription::Subscription(uint64_t ticket,
                             std::weak_ptr<Subscription::Source> source)
      : ticket_(ticket), source_wptr_(std::move(source)) {
    assert(source_wptr_.lock());
  }

  Subscription& Subscription::operator=(Subscription&& x) noexcept {
    cancel();
    ticket_ = x.ticket_;
    source_wptr_ = std::move(x.source_wptr_);
    return *this;
  }

  Subscription::~Subscription() {
    // cancels itself when going out-of-scope
    cancel();
  }

  void Subscription::cancel() {
    auto src = source_wptr_.lock();
    if (src) {
      src->unsubscribe(ticket_);
    }
    source_wptr_.reset();
  }

}  // namespace libp2p::protocol
