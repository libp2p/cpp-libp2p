/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_STORAGEIMPL
#define LIBP2P_PROTOCOL_KADEMLIA_STORAGEIMPL

#include <libp2p/protocol/kademlia/impl/storage.hpp>

#include <boost/multi_index/hashed_index_fwd.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index_fwd.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index_container_fwd.hpp>

#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/storage_backend.hpp>

namespace libp2p::protocol::kademlia {

  class StorageImpl : public Storage,
                      public std::enable_shared_from_this<StorageImpl> {
    struct ByKey;
    struct ByExpireTime;
    struct ByRefreshTime;

    // namespace mi = boost::multi_index;

    struct Record {
      ContentId key;
      scheduler::Ticks expire_time{};
      scheduler::Ticks refresh_time{};
      scheduler::Ticks updated_at{};
    };

    /// Table of Record indexed by key, expire time and refresh time
    using Table = boost::multi_index_container<
        Record,
        boost::multi_index::indexed_by<
            boost::multi_index::hashed_unique<
                boost::multi_index::tag<ByKey>,
                boost::multi_index::member<Record, ContentId, &Record::key>,
                std::hash<ContentId>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByExpireTime>,
                boost::multi_index::member<Record, scheduler::Ticks,
                                           &Record::expire_time>>,
            boost::multi_index::ordered_non_unique<
                boost::multi_index::tag<ByRefreshTime>,
                boost::multi_index::member<Record, scheduler::Ticks,
                                           &Record::refresh_time>>>>;

   public:
    StorageImpl(const Config &config, std::shared_ptr<StorageBackend> backend,
                std::shared_ptr<Scheduler> scheduler);

    ~StorageImpl() override;

    outcome::result<void> putValue(Key key, Value value) override;

    outcome::result<std::pair<Value, Time>> getValue(
        const Key &key) const override;

    bool hasValue(const Key &key) const override;

   private:
    void onRefreshTimer();

    const Config &config_;
    std::shared_ptr<StorageBackend> backend_;
    std::shared_ptr<Scheduler> scheduler_;

    std::unique_ptr<Table> table_;
    Scheduler::Handle refresh_timer_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_PROTOCOL_KADEMLIA_STORAGEIMPL
