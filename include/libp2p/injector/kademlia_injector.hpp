/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INJECTOR_KADEMLIAINJECTOR
#define LIBP2P_INJECTOR_KADEMLIAINJECTOR

#include <libp2p/injector/host_injector.hpp>

// implementations
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>
#include <libp2p/protocol/common/scheduler.hpp>
#include <libp2p/protocol/kad/impl/kad_impl.hpp>
#include <libp2p/protocol/kad/impl/local_value_store.hpp>
#include <libp2p/protocol/kad/impl/routing_table_impl.hpp>

#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/impl/kademlia_impl.hpp>
#include <libp2p/protocol/kademlia/impl/routing_table_impl.hpp>
#include <libp2p/protocol/kademlia/impl/value_store_backend_default.hpp>
#include <libp2p/protocol/kademlia/impl/value_store_impl.hpp>

namespace libp2p::injector {

  inline auto useOldKademliaConfig(
      const protocol::kad::KademliaConfig &config) {
    return boost::di::bind<protocol::kad::KademliaConfig>().template to(
        std::move(config))[boost::di::override];
  }

  inline auto useKademliaConfig(const protocol::kademlia::Config &config) {
    return boost::di::bind<protocol::kademlia::Config>().template to(
        std::move(config))[boost::di::override];
  }

  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  auto makeKademliaInjector(Ts &&... args) {
    return di::make_injector<InjectorConfig>(
        // clang-format off

				di::bind<protocol::SchedulerConfig>.template to(protocol::SchedulerConfig {}),
				di::bind<crypto::random::RandomGenerator>.template to<crypto::random::BoostRandomGenerator>(),
				di::bind<protocol::Scheduler>.template to<protocol::AsioScheduler>(),

				di::bind<protocol::kad::KademliaConfig>.template to(protocol::kad::KademliaConfig {}),
				di::bind<protocol::kad::RoutingTable>.template to<protocol::kad::RoutingTableImpl>(),
				di::bind<protocol::kad::Kad>.to([](auto const &inj) { return get_kad(inj); }),

				di::bind<protocol::kademlia::Config>.template to(protocol::kademlia::Config{}),
				di::bind<protocol::kademlia::RoutingTable>.template to<protocol::kademlia::RoutingTableImpl>(),
				di::bind<protocol::kademlia::ValueStoreBackend>.template to<protocol::kademlia::ValueStoreBackendDefault>(),
				di::bind<protocol::kademlia::ValueStore>.template to<protocol::kademlia::ValueStoreImpl>(),
				di::bind<protocol::kademlia::Kademlia>.template to<protocol::kademlia::KademliaImpl>(),


        // user-defined overrides...
        std::forward<decltype(args)>(args)...
        // clang-format on
    );
  }

}  // namespace libp2p::injector

#endif  // LIBP2P_INJECTOR_KADEMLIAINJECTOR
