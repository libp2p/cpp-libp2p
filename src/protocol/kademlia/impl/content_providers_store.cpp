/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

#include <libp2p/protocol/kademlia/impl/content_providers_store.hpp>

namespace libp2p::protocol::kademlia {

  namespace {
    constexpr size_t kMaxProvidersPerKey = 6; // TODO(artem): move to config
    constexpr scheduler::Ticks kCleanupTimerInterval = 120 * 1000;
  } //namespace

  ContentProvidersStore::ContentProvidersStore(Scheduler& scheduler,
      Scheduler::Ticks record_expiration) :
      scheduler_(scheduler),
      record_expiration_(record_expiration)
  {
    table_ = std::make_unique<cps_table::Table>();

    cleanup_timer_ = scheduler_.schedule(kCleanupTimerInterval,
        [this] { onCleanupTimer(); });
  }

  ContentProvidersStore::~ContentProvidersStore() = default;

  PeerIdVec ContentProvidersStore::getProvidersFor(
      const ContentAddress &key) const {
    PeerIdVec v;
    auto& idx = table_->get<ByKey>();
    auto [b, e] = idx.equal_range(key);
    for (auto it = b; it != e; ++it) {
      v.push_back(it->peer);
    }
    return v;
  }

  void ContentProvidersStore::addProvider(
      const ContentAddress& key, const peer::PeerId& peer) {

    auto expires = scheduler_.now() + record_expiration_;
    auto& idx = table_->get<ByKey>();
    auto [b, e] = idx.equal_range(key);
    auto oldest = b;
    auto equal = idx.end();
    size_t count = 0;
    for (auto it = b; it != e; ++it) {
      if (it->peer == peer) {
        equal = it;
        break;
      }
      ++count;
      if (it->expire_time < oldest->expire_time) {
        oldest = it;
      }
    }
    if (equal != idx.end()) {
      // provider refreshed itself, so do our host
      table_->modify(equal, [expires](cps_table::Record& r){ r.expire_time = expires; });
      return;
    }
    if (count >= kMaxProvidersPerKey) {
      idx.erase(oldest);
    }
    table_->insert( {key, peer, expires} );
  }

  void ContentProvidersStore::onCleanupTimer() {
    auto current_time = scheduler_.now();

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
        idx.erase(it);
      }
    }

    cleanup_timer_.reschedule(kCleanupTimerInterval);
  }

} //namespace
