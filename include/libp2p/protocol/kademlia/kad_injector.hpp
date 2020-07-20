/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_PROTOCOL_KADEMLIA_KAD_INJECTOR_HPP
#define LIBP2P_INCLUDE_LIBP2P_PROTOCOL_KADEMLIA_KAD_INJECTOR_HPP

#include <boost/di.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <libp2p/protocol/kademlia/impl/default_value_store.hpp>
#include <libp2p/protocol/kademlia/impl/kad_impl.hpp>
#include <libp2p/protocol/kademlia/impl/local_value_store.hpp>
#include <libp2p/protocol/kademlia/impl/routing_table_impl.hpp>

namespace libp2p::protocol::kademlia {

  inline auto useKademliaConfig(const KademliaConfig &kademlia_config) {
    return boost::di::bind<KademliaConfig>().template to(
        kademlia_config)[boost::di::override];
  }

  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  auto makeKadInjector(Ts &&... args) {
    using namespace boost;  // NOLINT

    KademliaConfig default_kad_config{};
    protocol::SchedulerConfig default_scheduler_config{};

    return di::make_injector<InjectorConfig>(
        di::bind<KademliaConfig>().template to(std::move(default_kad_config)),
        di::bind<protocol::SchedulerConfig>().template to(std::move(default_scheduler_config)),
        di::bind<Kad>().template to<KadImpl>(),
        di::bind<KadBackend>().template to<KadImpl>(),
        di::bind<Scheduler>().template to<AsioScheduler>(),
        di::bind<RoutingTable>().template to<RoutingTableImpl>(),
        di::bind<ValueStoreBackend>().template to<DefaultValueStore>(),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...);
  }  // namespace libp2p::protocol::kademlia

}  // namespace libp2p::protocol::kademlia

#endif  // LIBP2P_INCLUDE_LIBP2P_PROTOCOL_KADEMLIA_KAD_INJECTOR_HPP
