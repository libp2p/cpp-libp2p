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

  boost::optional<libp2p::peer::PeerInfo> str2peerInfo(const std::string &str) {
    auto server_ma_res = libp2p::multi::Multiaddress::create(str);
    if (!server_ma_res) {
      return boost::none;
    }
    auto server_ma = std::move(server_ma_res.value());

    auto server_peer_id_str = server_ma.getPeerId();
    if (!server_peer_id_str) {
      return boost::none;
    }

    auto server_peer_id_res = peer::PeerId::fromBase58(*server_peer_id_str);
    if (!server_peer_id_res) {
      return boost::none;
    }

    return peer::PeerInfo{server_peer_id_res.value(), {server_ma}};
  }

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
      std::shared_ptr<boost::asio::io_context> io) {
    std::pair<std::shared_ptr<Host>, std::shared_ptr<Gossip>> p;

    auto injector = makeInjector(
        boost::di::bind<boost::asio::io_context>.to(io)[boost::di::override]);

    p.first = injector.create<std::shared_ptr<Host>>();
    assert(p.first);

    p.second = std::make_shared<GossipCore>(std::move(config),
                                            std::move(scheduler), p.first);
    return p;
  }

}  // namespace libp2p::protocol::gossip::example
