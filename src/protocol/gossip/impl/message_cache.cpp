/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "message_cache.hpp"

#include <cassert>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <libp2p/common/hexutil.hpp>

#define TRACE_ENABLED 0
#include <libp2p/common/trace.hpp>

namespace libp2p::protocol::gossip {

  MessageCache::MessageCache(Time message_lifetime, TimeFunction clock)
      : message_lifetime_(message_lifetime), clock_(std::move(clock)) {
    assert(message_lifetime_ > Time::zero());
    table_ = std::make_unique<msg_cache_table::Table>();
  }

  MessageCache::~MessageCache() = default;

  bool MessageCache::contains(const MessageId &id) const {
    return table_->get<ById>().count(id) != 0;
  }

  boost::optional<TopicMessage::Ptr> MessageCache::getMessage(
      const MessageId &id) const {
    auto &idx = table_->get<ById>();
    auto it = idx.find(id);
    if (it == idx.end()) {
      TRACE("MessageCache: {} not found, current size {}",
            common::hex_upper(id), table_->size());
      return boost::none;
    }
    return it->message;
  }

  bool MessageCache::insert(TopicMessage::Ptr message,
                            const MessageId &msg_id) {
    if (!message || msg_id.empty()) {
      return false;
    }
    auto &idx = table_->get<ById>();
    auto it = idx.find(msg_id);
    if (it != idx.end()) {
      return false;
    }
    auto now = clock_();
    idx.insert({msg_id, now + message_lifetime_, std::move(message)});
    return true;
  }

  void MessageCache::shift() {
    auto &idx = table_->get<ByTimestamp>();
    if (idx.empty()) {
      return;
    }
    auto now = clock_();

    TRACE("MessageCache: size before shift: {}", table_->size());

    if (idx.rbegin()->expires_at < now) {
      table_->clear();
    } else {
      auto expired_until = idx.lower_bound(now);
      if (expired_until != idx.end()) {
        idx.erase(idx.begin(), expired_until);
      }
    }

    TRACE("MessageCache: size after shift: {}", table_->size());
  }

}  // namespace libp2p::protocol::gossip
