/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_GOSSIP_MESSAGE_CACHE_HPP
#define LIBP2P_PROTOCOL_GOSSIP_MESSAGE_CACHE_HPP

#include <functional>

#include <boost/multi_index/hashed_index_fwd.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index_fwd.hpp>
#include <boost/multi_index_container_fwd.hpp>

#include "common.hpp"

namespace libp2p::protocol::gossip {

  struct ById;
  struct ByTimestamp;

  namespace msg_cache_table {

    namespace mi = boost::multi_index;

    struct Record {
      MessageId message_id;
      Time expires_at;
      TopicMessage::Ptr message;
    };

    /// Table of Record with 2 indices (by key and expire time)
    using Table = boost::multi_index_container<
        Record,
        mi::indexed_by<
            // hashed_non_unique by id means unordered index
            mi::hashed_unique<
                mi::tag<ById>,
                mi::member<Record, MessageId, &Record::message_id>,
                std::hash<MessageId>>,
            // ordered_non_unique by time means accending order
            mi::ordered_non_unique<
                mi::tag<ByTimestamp>,
                mi::member<Record, Time, &Record::expires_at>>>>;

  }  // namespace msg_cache_table

  /// Message cache with expiration
  class MessageCache {
   public:
    /// External time function
    using TimeFunction = std::function<Time()>;

    MessageCache(Time message_lifetime,
                 TimeFunction clock);

    ~MessageCache();

    bool contains(const MessageId& id) const;

    /// Returns message by id if found
    boost::optional<TopicMessage::Ptr> getMessage(const MessageId &id) const;

    /// Inserts a new message into cache. If already there, returns false
    bool insert(TopicMessage::Ptr message, const MessageId& msg_id);

    /// Purges expired messages and updates seen notification data
    void shift();

   private:

    const Time message_lifetime_;
    TimeFunction clock_;
    std::unique_ptr<msg_cache_table::Table> table_;
  };

}  // namespace libp2p::protocol::gossip

#endif  // LIBP2P_PROTOCOL_GOSSIP_MESSAGE_CACHE_HPP
