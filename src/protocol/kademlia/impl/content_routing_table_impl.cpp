/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/kademlia/impl/content_routing_table_impl.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

namespace libp2p::protocol::kademlia {

  ContentRoutingTableImpl::ContentRoutingTableImpl(
      const Config &config, basic::Scheduler &scheduler,
      std::shared_ptr<event::Bus> bus)
      : config_(config), scheduler_(scheduler), bus_(std::move(bus)) {
    BOOST_ASSERT(bus_ != nullptr);
    table_ = std::make_unique<Table>();

    cleanup_timer_ = scheduler_.scheduleWithHandle([this] {
      cleanup_timer_ = scheduler_.scheduleWithHandle(
          [wp = weak_from_this()] {
            if (auto self = wp.lock()) {
              self->onCleanupTimer();
            }
          },
          config_.providerWipingInterval);
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
      if (limit > 0 and result.size() >= limit) {
        break;
      }
    }
    return result;
  }

  void ContentRoutingTableImpl::addProvider(const ContentId &key,
                                            const peer::PeerId &peer) {
    auto expires = scheduler_.now() + config_.providerRecordTTL;
    auto &idx = table_->get<ByKey>();
    auto [begin, end] = idx.equal_range(key);
    auto oldest = begin;
    auto equal = idx.end();
    size_t count = 0;
    for (auto it = begin; it != end; ++it, ++count) {
      if (it->peer == peer) {
        equal = it;
        break;
      }
      if (it->expire_time < oldest->expire_time) {
        oldest = it;
      }
    }
    if (equal != idx.end()) {
      // provider refreshed itself, so do our host
      table_->modify(equal, [expires](Record &r) { r.expire_time = expires; });
      return;
    }
    if (count >= config_.maxProvidersPerKey) {
      idx.erase(oldest);
    }
    table_->insert({key, peer, expires});
    bus_->getChannel<event::protocol::kademlia::ProvideContentChannel>()
        .publish({key, peer});
  }

  void ContentRoutingTableImpl::onCleanupTimer() {
    auto current_time = scheduler_.now();

    // cleanup expired records
    auto &idx = table_->get<ByExpireTime>();
    for (auto i = idx.begin(); i != idx.end();) {
      if (i->expire_time > current_time) {
        break;
      }
      auto ci = i++;
      idx.erase(ci);
    }

    std::ignore = cleanup_timer_.reschedule(config_.providerWipingInterval);
  }

}  // namespace libp2p::protocol::kademlia
