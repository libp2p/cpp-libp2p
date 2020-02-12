/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KADEMLIA_LOCAL_VALUE_STORE_HPP
#define LIBP2P_KADEMLIA_LOCAL_VALUE_STORE_HPP

#include <boost/multi_index/hashed_index_fwd.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index_fwd.hpp>
#include <boost/multi_index_container_fwd.hpp>

#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/kademlia/value_store_backend.hpp>
#include <libp2p/protocol/kademlia/impl/kad_backend.hpp>

namespace libp2p::protocol::kademlia {

  std::unique_ptr<ValueStoreBackend> createDefaultValueStoreBackend();

  struct ByKey;
  struct ByExpireTime;
  struct ByRefreshTime;

  namespace lvs_table {

    namespace mi = boost::multi_index;

    struct Record {
      ContentAddress key;
      scheduler::Ticks expire_time = scheduler::Ticks{};
      scheduler::Ticks refresh_time = scheduler::Ticks{};
      scheduler::Ticks updated_at = scheduler::Ticks{};
    };

    /// Table of Record indexed by key, expire time and refresh time
    using Table = boost::multi_index_container<
        Record,
        mi::indexed_by<
            mi::hashed_unique<mi::tag<ByKey>,
                              mi::member<Record, ContentAddress, &Record::key>,
                                  std::hash<ContentAddress>>,
            mi::ordered_non_unique<
                mi::tag<ByExpireTime>,
                mi::member<Record, scheduler::Ticks, &Record::expire_time>>,
            mi::ordered_non_unique<
                mi::tag<ByRefreshTime>,
                mi::member<Record, scheduler::Ticks, &Record::refresh_time>>>>;

  }  // namespace lvs_table

  /// Manages key-value pairs stored on this host, and their lifetime (expiration)
  class LocalValueStore {
   public:
    using Ticks = scheduler::Ticks;
    using AbsTime = Ticks;

    explicit LocalValueStore(KadBackend& kad);

    ~LocalValueStore();

    [[nodiscard]] bool has(const ContentAddress &key) const;

    outcome::result<void> putValue(const ContentAddress &key, Value value);

    outcome::result<Value> getValue(const ContentAddress &key,
                                    AbsTime &timestamp) const;

   private:
    void onRefreshTimer();

    KadBackend &kad_;
    const Ticks max_record_age_;
    const Ticks refresh_interval_;
    std::unique_ptr<ValueStoreBackend> local_storage_;
    std::unique_ptr<lvs_table::Table> table_;
    Scheduler::Handle refresh_timer_;
  };
}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KADEMLIA_LOCAL_VALUE_STORE_HPP
