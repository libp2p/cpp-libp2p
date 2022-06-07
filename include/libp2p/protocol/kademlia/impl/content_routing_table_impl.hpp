/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_KADEMLIA_CONTENTROUTINGIMPL
#define LIBP2P_PROTOCOL_KADEMLIA_CONTENTROUTINGIMPL

#include <libp2p/protocol/kademlia/impl/content_routing_table.hpp>

#include <boost/multi_index/hashed_index_fwd.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index_fwd.hpp>
#include <boost/multi_index_container_fwd.hpp>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/kademlia/config.hpp>

namespace libp2p::protocol::kademlia {

  struct ByKey;
  struct ByExpireTime;

  namespace {

    namespace mi = boost::multi_index;

    struct Record {
      ContentId key;
      peer::PeerId peer;
      Time expire_time = Time::zero();
    };

    /// Table of Record with 2 indices (by key and expire time)
    using Table = boost::multi_index_container<
        Record,
        mi::indexed_by<
            // hashed_non_unique by key means unordered index, non-unique
            mi::hashed_non_unique<mi::tag<ByKey>,
                                  mi::member<Record, ContentId, &Record::key>,
                                  std::hash<ContentId>>,
            // ordered_non_unique by expire time means accending order
            mi::ordered_non_unique<
                mi::tag<ByExpireTime>,
                mi::member<Record, Time, &Record::expire_time>>>>;

  }  // namespace

  class ContentRoutingTableImpl
      : public ContentRoutingTable,
        public std::enable_shared_from_this<ContentRoutingTableImpl> {
   public:
    ContentRoutingTableImpl(const Config &config, basic::Scheduler &scheduler,
                            std::shared_ptr<event::Bus> bus);

    ~ContentRoutingTableImpl() override;

    std::vector<PeerId> getProvidersFor(const ContentId &key,
                                        size_t limit = 0) const override;

    void addProvider(const ContentId &key, const peer::PeerId &peer) override;

   private:
    void onCleanupTimer();

    const Config &config_;
    basic::Scheduler &scheduler_;
    std::shared_ptr<event::Bus> bus_;
    std::unique_ptr<Table> table_;
    basic::Scheduler::Handle cleanup_timer_;
  };

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_KAD_CONTENT_PROVIDERS_STORE_HPP
