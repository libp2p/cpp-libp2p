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
#include <libp2p/protocol/kademlia/config.hpp>
#include <libp2p/protocol/kademlia/impl/content_routing_table_impl.hpp>
#include <libp2p/protocol/kademlia/impl/kademlia_impl.hpp>
#include <libp2p/protocol/kademlia/impl/peer_routing_table_impl.hpp>
#include <libp2p/protocol/kademlia/impl/storage_backend_default.hpp>
#include <libp2p/protocol/kademlia/impl/storage_impl.hpp>
#include <libp2p/protocol/kademlia/impl/validator_default.hpp>

namespace libp2p::injector {

  template <
      typename T, typename C = std::decay_t<T>,
      typename = std::enable_if<std::is_same_v<C, protocol::kademlia::Config>>>
  inline auto useKademliaConfig(T &&c) {
    return boost::di::bind<C>().template to(
        [c = std::forward<C>(c)](const auto &injector) mutable -> const C & {
          static C instance = std::move(c);
          return instance;
        })[boost::di::override];
  }

  // clang-format off
  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  auto makeKademliaInjector(Ts &&... args) {
    using namespace boost;  // NOLINT
    return di::make_injector<InjectorConfig>(
        // clang-format off

        di::bind<protocol::SchedulerConfig>.template to(protocol::SchedulerConfig {}),
        di::bind<crypto::random::RandomGenerator>.template to<crypto::random::BoostRandomGenerator>(),
        di::bind<protocol::Scheduler>.template to<protocol::AsioScheduler>(),

        di::bind<protocol::kademlia::Config>.template to<protocol::kademlia::Config>().in(di::singleton),
        di::bind<protocol::kademlia::ContentRoutingTable>.template to<protocol::kademlia::ContentRoutingTableImpl>(),
        di::bind<protocol::kademlia::PeerRoutingTable>.template to<protocol::kademlia::PeerRoutingTableImpl>(),
        di::bind<protocol::kademlia::StorageBackend>.template to<protocol::kademlia::StorageBackendDefault>(),
        di::bind<protocol::kademlia::Storage>.template to<protocol::kademlia::StorageImpl>(),
        di::bind<protocol::kademlia::Validator>.template to<protocol::kademlia::ValidatorDefault>(),

        di::bind<protocol::kademlia::MessageObserver>.template to<protocol::kademlia::KademliaImpl>().in(di::singleton),
        di::bind<protocol::kademlia::Kademlia>.template to<protocol::kademlia::KademliaImpl>().in(di::singleton),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...
        // clang-format on
    );
  }

}  // namespace libp2p::injector

#endif  // LIBP2P_INJECTOR_KADEMLIAINJECTOR
