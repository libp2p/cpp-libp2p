/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_VALUESTOREIMPL
#define LIBP2P_PROTOCOL_KADEMLIA_VALUESTOREIMPL

#include <libp2p/protocol/kademlia/value_store.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/value_store_backend.hpp>

namespace libp2p::protocol::kademlia {

  namespace {

    struct ByKey;
    struct ByExpireTime;
    struct ByRefreshTime;

    namespace mi = boost::multi_index;

    struct Record {
      ContentId key;
      scheduler::Ticks expire_time{};
      scheduler::Ticks refresh_time{};
      scheduler::Ticks updated_at{};
    };

    /// Table of Record indexed by key, expire time and refresh time
    using Table = boost::multi_index_container<
        Record,
        mi::indexed_by<
            mi::hashed_unique<mi::tag<ByKey>,
                              mi::member<Record, ContentId, &Record::key>,
                              std::hash<ContentId>>,
            mi::ordered_non_unique<
                mi::tag<ByExpireTime>,
                mi::member<Record, scheduler::Ticks, &Record::expire_time>>,
            mi::ordered_non_unique<
                mi::tag<ByRefreshTime>,
                mi::member<Record, scheduler::Ticks, &Record::refresh_time>>>>;

  }  // namespace

  class ValueStoreImpl : public ValueStore,
                         public std::enable_shared_from_this<ValueStoreImpl> {
   public:
    ValueStoreImpl(const Config &config,
                   std::shared_ptr<ValueStoreBackend> backend,
                   std::shared_ptr<Scheduler> scheduler);

    ~ValueStoreImpl() override = default;

    outcome::result<void> putValue(Key key, Value value) override;
    outcome::result<std::pair<Value, Time>> getValue(
        const Key &key) const override;

   private:
    void onRefreshTimer();

    const Config &config_;
    std::shared_ptr<ValueStoreBackend> backend_;
    std::shared_ptr<Scheduler> scheduler_;

    std::unique_ptr<Table> table_;
    Scheduler::Handle refresh_timer_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_VALUESTOREIMPL
