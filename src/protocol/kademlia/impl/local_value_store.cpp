/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/local_value_store.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <unordered_map>

#include <libp2p/protocol/kademlia/config.hpp>

namespace libp2p::protocol::kademlia {

  namespace {
    struct DefaultValueStore : public ValueStoreBackend {
      DefaultValueStore() = default;
      ~DefaultValueStore() override = default;

      /// PutValue adds value corresponding to given Key.
      bool putValue(ContentAddress key, Value value) override {
        values_[std::move(key)] = std::move(value);
        return true; // N.B. Real backend should validate key and value
      }

      /// GetValue searches for the value corresponding to given Key.
      [[nodiscard]] outcome::result<Value> getValue(
          const ContentAddress &key) const override {
        auto it = values_.find(key);
        if (it == values_.end()) {
          return Error::VALUE_NOT_FOUND;
        }
        return it->second;
      }

      void erase(const ContentAddress &key) override {
        values_.erase(key);
      }

     private:
      std::unordered_map<ContentAddress, Value> values_;
    };

    constexpr scheduler::Ticks kRefreshTimerInterval = 60 * 1000;

    constexpr int kRefreshN = 2;
    constexpr int kRefreshD = 5;

  }  // namespace

  std::unique_ptr<ValueStoreBackend> createDefaultValueStoreBackend() {
    return std::make_unique<DefaultValueStore>();
  }

  LocalValueStore::LocalValueStore(KadBackend& kad)
      : kad_(kad),
        max_record_age_(scheduler::toTicks(kad_.config().max_record_age)),
        refresh_interval_(max_record_age_ * kRefreshN / kRefreshD)

  {
    assert(refresh_interval_ > kRefreshTimerInterval);

    table_ = std::make_unique<lvs_table::Table>();

    refresh_timer_ = kad_.scheduler().schedule(kRefreshTimerInterval,
                                         [this] { onRefreshTimer(); });
  }

  LocalValueStore::~LocalValueStore() = default;

  bool LocalValueStore::has(const ContentAddress &key) const {
    return table_->get<ByKey>().count(key) > 0;
  }

  outcome::result<void> LocalValueStore::putValue(const ContentAddress &key,
                                 Value value) {
    if (!local_storage_->putValue(key, std::move(value))) {
      return Error::CONTENT_VALIDATION_FAILED;
    }

    auto current_time = kad_.scheduler().now();
    auto &idx = table_->get<ByKey>();
    auto it = idx.find(key);
    if (it == idx.end()) {
      // new k-v pair, needs initial advertise
      kad_.broadcastThisProvider(key);
      table_->insert({key, current_time + max_record_age_,
                     current_time + refresh_interval_, current_time});
    } else {
      // updated or refreshed value, reschedule expiration
      table_->modify(it, [current_time, this](lvs_table::Record &r) {
        r.expire_time = current_time + max_record_age_;
        r.updated_at = current_time;
      });
    }

    return outcome::success();
  }

  outcome::result<Value> LocalValueStore::getValue(
      const ContentAddress &key, LocalValueStore::AbsTime &timestamp) const {
    auto &idx = table_->get<ByKey>();
    auto it = idx.find(key);
    if (it == idx.end()) {
      return Error::VALUE_NOT_FOUND;
    }
    timestamp = it->updated_at;
    return local_storage_->getValue(key);
  }

  void LocalValueStore::onRefreshTimer() {
    auto current_time = kad_.scheduler().now();

    for (;;) {
      // cleanup expired records
      auto &idx = table_->get<ByExpireTime>();
      if (idx.empty()) {
        break;
      }
      for (;;) {
        auto it = idx.begin();
        if (it->expire_time > current_time) {
          break;
        }
        local_storage_->erase(it->key);
        idx.erase(it);
      }
    }

    for (;;) {
      // refresh if time arrived
      auto &idx = table_->get<ByRefreshTime>();
      if (idx.empty()) {
        break;
      }
      for (;;) {
        auto it = idx.begin();
        if (it->refresh_time > current_time) {
          break;
        }
        kad_.broadcastThisProvider(it->key);
        idx.modify(it, [t = refresh_interval_](auto &record) {
          record.refresh_time += t;
        });
      }
    }

    refresh_timer_.reschedule(kRefreshTimerInterval);
  }

}  // namespace libp2p::protocol::kademlia
