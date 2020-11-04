/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/content_routing_table_impl.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

namespace libp2p::protocol::kademlia {

  ContentRoutingTableImpl::ContentRoutingTableImpl(const Config &config,
                                                   Scheduler &scheduler)
      : scheduler_(scheduler),
        record_expiration_(scheduler::toTicks(config.providerRecordTTL)),
        cleanup_timer_interval_(
            scheduler::toTicks(config.providerWipingInterval)),
        max_providers_per_key_(config.maxProvidersPerKey) {
    table_ = std::make_unique<Table>();

    cleanup_timer_ = scheduler_.schedule([this] {
      cleanup_timer_ =
          scheduler_.schedule(cleanup_timer_interval_, [wp = weak_from_this()] {
            if (auto self = wp.lock()) {
              self->onCleanupTimer();
            }
          });
    });
  }

  ContentRoutingTableImpl::~ContentRoutingTableImpl() = default;

  std::vector<PeerId> ContentRoutingTableImpl::getProvidersFor(
      const ContentId &key, size_t limit) const {
    std::vector<PeerId> result;
    auto &idx = table_->get<ByKey>();
    auto [begin, end] = idx.equal_range(key);
    for (auto it = begin; it != end; ++it) {
      result.push_back(it->peer);
      if (result.size() >= limit) {
        break;
      }
    }
    return result;
  }

  void ContentRoutingTableImpl::addProvider(const ContentId &key,
                                            const peer::PeerId &peer) {
    auto expires = scheduler_.now() + record_expiration_;
    auto &idx = table_->get<ByKey>();
    auto [begin, end] = idx.equal_range(key);
    auto oldest = begin;
    auto equal = idx.end();
    size_t count = 0;
    for (auto it = begin; it != end; ++it) {
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
      table_->modify(equal, [expires](Record &r) { r.expire_time = expires; });
      return;
    }
    if (count >= max_providers_per_key_) {
      idx.erase(oldest);
    }
    table_->insert({key, peer, expires});
  }

  void ContentRoutingTableImpl::onCleanupTimer() {
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

    cleanup_timer_.reschedule(cleanup_timer_interval_);
  }

}  // namespace libp2p::protocol::kademlia
