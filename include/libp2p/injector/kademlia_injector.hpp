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

namespace libp2p::injector {

  inline auto useKademliaConfig(
      const protocol::kad::KademliaConfig &config) {
    return boost::di::bind<protocol::kad::KademliaConfig>().template to(
        std::move(config))[boost::di::override];
  }

  // sr25519 kp getter
  template <typename Injector>
  std::shared_ptr<libp2p::protocol::kad::Kad> get_kad(
      const Injector &injector) {
    static auto initialized =
        boost::optional<std::shared_ptr<libp2p::protocol::kad::Kad>>(
            boost::none);
    if (initialized) {
      return initialized.value();
    }

    auto host = injector.template create<std::shared_ptr<Host>>();

    auto scheduler =
        injector.template create<std::shared_ptr<protocol::Scheduler>>();
    auto table = injector.template create<
        std::shared_ptr<protocol::kad::RoutingTable>>();
    auto storage = injector.template create<
        std::unique_ptr<protocol::kad::ValueStoreBackend>>();
    auto config =
        injector.template create<protocol::kad::KademliaConfig>();
    auto random_generator = injector.template create<
        std::shared_ptr<crypto::random::RandomGenerator>>();

    initialized = std::make_shared<libp2p::protocol::kad::KadImpl>(
        std::move(host), std::move(scheduler), std::move(table),
        std::move(storage), config, std::move(random_generator));
    return initialized.value();
  }

  // clang-format off
template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
auto makeKademliaInjector(Ts &&... args) {
	using namespace boost;  // NOLINT

	// clang-format off
	  return di::make_injector<InjectorConfig>(

			  di::bind<protocol::SchedulerConfig>.template to(protocol::SchedulerConfig {}),
			  di::bind<crypto::random::RandomGenerator>.template to<crypto::random::BoostRandomGenerator>(),
			  di::bind<protocol::Scheduler>.template to<protocol::AsioScheduler>(),

			  di::bind<protocol::kad::KademliaConfig>.template to(protocol::kad::KademliaConfig {}),
			  di::bind<protocol::kad::RoutingTable>.template to<protocol::kad::RoutingTableImpl>(),
			  di::bind<protocol::kad::Kad>.to([](auto const &inj) { return get_kad(inj); }),

			  // user-defined overrides...
			  std::forward<decltype(args)>(args)...
	  );
	  // clang-format on
  }

}  // namespace libp2p::injector

#endif  // LIBP2P_INJECTOR_KADEMLIAINJECTOR
