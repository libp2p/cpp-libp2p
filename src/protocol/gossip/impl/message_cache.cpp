/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/gossip/impl/message_cache.hpp>

#include <cassert>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

namespace libp2p::protocol::gossip {

  MessageCache::MessageCache(Time message_lifetime, TimeFunction clock)
      : message_lifetime_(message_lifetime), clock_(std::move(clock)) {
    assert(message_lifetime_ > 0);
    table_ = std::make_unique<msg_cache_table::Table>();
  }

  MessageCache::~MessageCache() = default;

  bool MessageCache::contains(const MessageId& id) const {
    return table_->get<ById>().count(id) != 0;
  }

  boost::optional<TopicMessage::Ptr> MessageCache::getMessage(
      const MessageId &id) const {
    auto &idx = table_->get<ById>();
    auto it = idx.find(id);
    if (it == idx.end()) {
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
    idx.insert({msg_id, now, std::move(message)});
    return true;
  }

  void MessageCache::shift() {
    auto &idx = table_->get<ByTimestamp>();
    if (idx.empty()) {
      return;
    }
    auto now = clock_();
    auto cache_expires = now - message_lifetime_;

    auto expired_until = idx.lower_bound(cache_expires);

    if (expired_until != idx.end()) {
      idx.erase(idx.begin(), expired_until);
    } else {
      table_->clear();
    }
  }

}  // namespace libp2p::protocol::gossip
