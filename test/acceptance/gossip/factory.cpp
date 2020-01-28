/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "factory.hpp"

#include <cassert>

#include <boost/di/extension/scopes/shared.hpp>

#include <libp2p/injector/host_injector.hpp>
#include <libp2p/protocol/gossip/impl/gossip_core.hpp>

namespace libp2p::protocol::gossip::example {

  namespace {
    template <typename... Ts>
    auto makeInjector(Ts &&... args) {
      return libp2p::injector::makeHostInjector<
          boost::di::extension::shared_config>(
          std::forward<decltype(args)>(args)...);
    }

  }  // namespace

  std::pair<std::shared_ptr<Host>, std::shared_ptr<Gossip>> createHostAndGossip(
      Config config, std::shared_ptr<Scheduler> scheduler,
      std::shared_ptr<boost::asio::io_context> io,
      boost::optional<crypto::KeyPair> keypair) {
    std::pair<std::shared_ptr<Host>, std::shared_ptr<Gossip>> p;

    if (!keypair) {
      auto injector = makeInjector(
          boost::di::bind<boost::asio::io_context>.to(io)[boost::di::override]);
      p.first = injector.create<std::shared_ptr<Host>>();
    } else {
      auto injector = makeInjector(
          boost::di::bind<boost::asio::io_context>.to(io)[boost::di::override],
          boost::di::bind<crypto::KeyPair>().template to(
              std::move(keypair.value()))[boost::di::override]
          //, boost::di::bind<muxer::MuxerAdaptor *[]>().template to<muxer::Mplex>()
          //[boost::di::override]
                  );
      p.first = injector.create<std::shared_ptr<Host>>();
    }

    assert(p.first);

    p.second = std::make_shared<GossipCore>(std::move(config),
                                            std::move(scheduler), p.first);
    return p;
  }

}  // namespace libp2p::protocol::gossip::example
