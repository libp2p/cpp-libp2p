/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/storage_impl.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <unordered_map>

#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/error.hpp>

namespace libp2p::protocol::kademlia {

  StorageImpl::StorageImpl(const Config &config,
                           std::shared_ptr<StorageBackend> backend,
                           std::shared_ptr<basic::Scheduler> scheduler)
      : config_(config),
        backend_(std::move(backend)),
        scheduler_(std::move(scheduler)) {
    BOOST_ASSERT(backend_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    BOOST_ASSERT(config_.storageRecordTTL > config_.storageWipingInterval);

    table_ = std::make_unique<Table>();

    refresh_timer_ = scheduler_->scheduleWithHandle(
        [this] { onRefreshTimer(); }, config_.storageWipingInterval);
  }

  StorageImpl::~StorageImpl() = default;

  outcome::result<void> StorageImpl::putValue(Key key, Value value) {
    OUTCOME_TRY(backend_->putValue(key, value));

    auto now = scheduler_->now();
    auto expire_time = now + config_.storageRecordTTL;

    auto &idx = table_->get<ByKey>();

    if (auto it = idx.find(key); it == idx.end()) {
      table_->insert({key, expire_time, now});
    } else {
      table_->modify(it, [expire_time, now](auto &record) {
        record.expire_time = expire_time;
        record.updated_at = now;
      });
    }

    return outcome::success();
  }

  outcome::result<ValueAndTime> StorageImpl::getValue(const Key &key) const {
    auto &idx = table_->get<ByKey>();
    auto it = idx.find(key);
    if (it == idx.end()) {
      return Error::VALUE_NOT_FOUND;
    }
    OUTCOME_TRY(value, backend_->getValue(key));
    return {value, it->expire_time};
  }

  bool StorageImpl::hasValue(const Key &key) const {
    auto &idx = table_->get<ByKey>();
    auto it = idx.find(key);
    if (it == idx.end()) {
      return false;
    }
    return it->expire_time > scheduler_->now();
  }

  void StorageImpl::onRefreshTimer() {
    auto now = scheduler_->now();

    // cleanup expired records
    auto &idx_by_expiration = table_->get<ByExpireTime>();
    for (auto i = idx_by_expiration.begin(); i != idx_by_expiration.end();) {
      if (i->expire_time > now) {
        break;
      }

      auto ci = i++;
      if (backend_->erase(ci->key)) {
        idx_by_expiration.erase(ci);
      }
    }

    // refresh if time arrived
    auto &idx_by_refresing = table_->get<ByRefreshTime>();
    for (auto i = idx_by_refresing.begin(); i != idx_by_refresing.end(); ++i) {
      if (i->refresh_time > now) {
        break;
      }

      idx_by_refresing.modify(i, [this](auto &record) {
        record.refresh_time += config_.storageRefreshInterval;
      });
    }

    std::ignore = refresh_timer_.reschedule(config_.storageRefreshInterval);
  }

}  // namespace libp2p::protocol::kademlia
