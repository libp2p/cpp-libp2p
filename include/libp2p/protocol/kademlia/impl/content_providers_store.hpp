/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_KAD_CONTENT_PROVIDERS_STORE_HPP
#define LIBP2P_KAD_CONTENT_PROVIDERS_STORE_HPP

#include <boost/multi_index/hashed_index_fwd.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index_fwd.hpp>
#include <boost/multi_index_container_fwd.hpp>

#include <libp2p/protocol/kademlia/common.hpp>
#include <libp2p/protocol/common/scheduler.hpp>

namespace libp2p::protocol::kademlia {

  struct ByKey;
  struct ByExpireTime;

  namespace cps_table {

    namespace mi = boost::multi_index;

    struct Record {
      ContentAddress key;
      peer::PeerId peer;
      scheduler::Ticks expire_time = scheduler::Ticks{};
    };

    /// Table of Record with 2 indices (by key and expire time)
    using Table = boost::multi_index_container<
        Record,
        mi::indexed_by<
            // hashed_non_unique by key means unordered index, non-unique
            mi::hashed_non_unique<mi::tag<ByKey>,
                mi::member<Record, ContentAddress, &Record::key>,
                std::hash<ContentAddress>>,
            // ordered_non_unique by expire time means accending order
            mi::ordered_non_unique<
                mi::tag<ByExpireTime>,
                mi::member<Record, scheduler::Ticks, &Record::expire_time>>>>;

  }  // namespace lvs_table

  class ContentProvidersStore {
   public:
    using Ticks = scheduler::Ticks;
    using AbsTime = Ticks;

    ContentProvidersStore(Scheduler& scheduler, Scheduler::Ticks record_expiration);

    ~ContentProvidersStore();

    PeerIdVec getProvidersFor(const ContentAddress& key) const;

    void addProvider(const ContentAddress& key, const peer::PeerId& peer);

   private:

    void onCleanupTimer();

    Scheduler& scheduler_;
    const Scheduler::Ticks record_expiration_;
    std::unique_ptr<cps_table::Table> table_;
    Scheduler::Handle cleanup_timer_;
  };

} //namespace

#endif  // LIBP2P_KAD_CONTENT_PROVIDERS_STORE_HPP
