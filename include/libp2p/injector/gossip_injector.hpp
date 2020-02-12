/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GOSSIP_INJECTOR_HPP
#define LIBP2P_GOSSIP_INJECTOR_HPP

#include <libp2p/injector/host_injector.hpp>

// implementations
#include <libp2p/protocol/gossip/impl/gossip_core.hpp>
#include <libp2p/protocol/common/asio/asio_scheduler.hpp>

namespace libp2p::injector {

  auto useGossipConfig(const protocol::gossip::Config& c) {
    return boost::di::bind<protocol::gossip::Config>().template to(
        c)[boost::di::override];
  }

  // clang-format off
  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  auto makeGossipInjector(Ts &&... args) {
    using namespace boost;  // NOLINT

    // clang-format off
    return di::make_injector<InjectorConfig>(
        makeHostInjector<InjectorConfig>(),

        di::bind<protocol::gossip::Config>.template to(protocol::gossip::Config {}),
        di::bind<protocol::SchedulerConfig>.template to(protocol::SchedulerConfig {}),
        di::bind<protocol::gossip::Gossip>.template to<protocol::gossip::GossipCore>(),
        di::bind<protocol::Scheduler>.template to<protocol::AsioScheduler>(),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...
    );
    // clang-format on
  }

}  // namespace libp2p::injector

#endif  // LIBP2P_GOSSIP_INJECTOR_HPP
