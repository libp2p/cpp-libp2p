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

  MessageCache::MessageCache(Time message_lifetime, Time broadcast_lifetime,
                             TimeFunction clock)
      : message_lifetime_(message_lifetime),
        broadcast_lifetime_(broadcast_lifetime),
        clock_(std::move(clock)),
        oldest_in_seen_(0) {
    assert(broadcast_lifetime_ <= message_lifetime_);
    table_ = std::make_unique<msg_cache_table::Table>();
  }

  MessageCache::~MessageCache() = default;

  void MessageCache::getSeenMessageIds(const TopicId &topic,
                                       const IHaveCallback &callback) const {
    bool by_topic = not topic.empty();
    auto it = by_topic ? seen_by_topic_.upper_bound({topic, MessageId{}})
                       : seen_by_topic_.begin();
    for (; it != seen_by_topic_.end(); ++it) {
      if (by_topic && it->first != topic) {
        break;
      }
      callback(it->first, it->second);
    }
  }

  TopicMessage::Ptr MessageCache::getMessage(const MessageId &id) const {
    auto &idx = table_->get<ById>();
    auto it = idx.find(id);
    if (it == idx.end()) {
      return TopicMessage::Ptr{};
    }
    return it->message;
  }

  boost::optional<MessageId> MessageCache::insert(TopicMessage::Ptr message) {
    if (!message) {
      return {};
    }
    MessageId id = createMessageId(*message);
    auto &idx = table_->get<ById>();
    auto it = idx.find(id);
    if (it != idx.end()) {
      return {};
    }
    for (const auto &topic : message->topic_ids) {
      seen_by_topic_.insert({topic, id});
    }
    auto now = clock_();
    if (oldest_in_seen_ == 0)
      oldest_in_seen_ = now;
    idx.insert({id, now, std::move(message)});
    return boost::optional<MessageId>(std::move(id));
  }

  void MessageCache::shift() {
    auto &idx = table_->get<ByTimestamp>();
    if (idx.empty()) {
      return;
    }
    auto now = clock_();
    auto cache_expires = now - message_lifetime_;
    auto seen_expires = now - broadcast_lifetime_;

    assert(oldest_in_seen_ > 0);

    if (oldest_in_seen_ < seen_expires) {
      auto start = idx.lower_bound(oldest_in_seen_);

      assert(start != idx.end());

      auto stop = idx.upper_bound(seen_expires);
      for (auto it = start; it != stop; ++it) {
        // shift seen table
        for (const auto &topic : it->message->topic_ids) {
          seen_by_topic_.erase({topic, it->message_id});
        }
      }
      if (seen_by_topic_.empty()) {
        // everything has expired
        table_->clear();
        oldest_in_seen_ = 0;

        return;
      }

      assert(stop != idx.end());

      oldest_in_seen_ = stop->inserted_at;
    } else {
      // nothing expired
      return;
    }

    auto expired_until = idx.lower_bound(cache_expires);

    assert(expired_until != idx.end());

    idx.erase(idx.begin(), expired_until);
  }

}  // namespace libp2p::protocol::gossip
