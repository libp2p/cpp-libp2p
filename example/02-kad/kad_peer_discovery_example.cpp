/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <libp2p/common/literals.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>
#include <libp2p/network/connection_manager.hpp>
#include "../../src/protocol/kademlia/kad2/kad2_common.hpp"
#include "../../src/protocol/kademlia/kad2/low_res_timer.hpp"
#include "factory.hpp"

using namespace libp2p::kad_example;
using namespace libp2p::kad2;
static const uint16_t kPortBase = 40000;

libp2p::common::Logger logger;

struct Hosts {
  struct Host {
    size_t index;
    std::shared_ptr<libp2p::Host> host;
    std::shared_ptr<Kad> kad;
    std::string listen_to;
    std::string connect_to;
    LowResTimer* timer = nullptr;
    std::optional<libp2p::peer::PeerId> find_id;
    LowResTimer::Handle htimer;
    bool found = false;
//    libp2p::event::Handle new_channel_subscription;

    Host(size_t i, std::shared_ptr<libp2p::Host> h, RoutingTablePtr rt) : index(i), host(std::move(h))
    {
      kad = createDefaultKadImpl(host, std::move(rt));

    }

    void checkPeers() {
      auto peers = host->getPeerRepository().getPeers();
      logger->info("host {}: peers in repo: {}, found: {}", index, peers.size(), found);
    }

    void listen(std::shared_ptr<boost::asio::io_context>& io) {
      if (listen_to.empty()) return;
      io->post([this] { onListen(); });
    }

    void onListen() {
      auto ma =
        libp2p::multi::Multiaddress::create(listen_to).value();
      auto listen_res = host->listen(ma);
      if (!listen_res) {
        logger->error("Server {} cannot listen the given multiaddress: {}", index, listen_res.error().message());
      }

      host->start();
      logger->info("server {} listening to: {} peerId={}", index, ma.getStringAddress(), host->getPeerInfo().id.toBase58());

      kad->start(true);
    }

    void connect() {
      if (connect_to.empty()) return;
      auto pi = str2peerInfo(connect_to);

//      if (pi) {
//        host->connect(pi.value());
//      }

      kad->addPeer(std::move(pi.value()), true);
    }

    void findPeer(LowResTimer& t, const libp2p::peer::PeerId& id) {
      find_id = id;
      timer = &t;
      htimer = timer->create(200, [this] {
        kad->findPeer(find_id.value(), [this](const libp2p::peer::PeerId& peer, Kad::FindPeerQueryResult res) {
          onFindPeer(peer, std::move(res));
        });
      });
    }

    void onFindPeer(const libp2p::peer::PeerId& peer, Kad::FindPeerQueryResult res) {
      logger->info("onFindPeer: i={}, res: success={}, peers={}", index, res.success, res.closer_peers.size());
      if (res.success) {
        found = true;
        return;
      }
      htimer = timer->create(1200, [this] {
        kad->findPeer(find_id.value(), [this](const libp2p::peer::PeerId& peer, Kad::FindPeerQueryResult res) {
          onFindPeer(peer, std::move(res));
        });
      });
    }
  };

  std::vector<Host> hosts;

  Hosts(size_t n) {
    hosts.reserve(n);

    for (size_t i=0; i<n; ++i) {
      newHost();
    }

    makeConnectTopologyCircle();
  }

  void newHost() {
    size_t index = hosts.size();
    std::shared_ptr<libp2p::Host> host;
    RoutingTablePtr table;
    createHostAndRoutingTable(host, table);
    assert(host && table);
    libp2p::kad_example::createHostAndRoutingTable(host, table);
    hosts.emplace_back(index, std::move(host), std::move(table));
  }

  void makeConnectTopologyCircle() {
    for (auto& h : hosts) {
      h.listen_to = std::string("/ip4/127.0.0.1/tcp/") + std::to_string(kPortBase + h.index);
    }
    for (auto& h : hosts) {
      Host& server = hosts[(h.index > 0) ? h.index-1 : hosts.size()-1];
      if (!server.listen_to.empty()) {
        h.connect_to = server.listen_to + "/ipfs/" + server.host->getId().toBase58();
      }
    }
  }

  void listen(std::shared_ptr<boost::asio::io_context>& io) {
    for (auto& h : hosts) {
      h.listen(io);
    }
  }

  void connect() {
    for (auto& h : hosts) {
      h.connect();
    }
  }

  void findPeers(LowResTimer& timer) {
    for (auto& h : hosts) {
      auto half = hosts.size() / 2;
      auto index = h.index;
      if (index > half) {
        index -= half;
      } else {
        index += (half - 1);
      }
      Host& server = hosts[index];
      h.findPeer(timer, server.host->getId());
    }
  }

  void checkPeers() {
    for (auto& h : hosts) {
      h.checkPeers();
    }
  }
};

void setupLoggers(bool kad_log_debug) {
  static const char* kPattern = "%L %T.%e %v";

  logger = libp2p::common::createLogger("kad-example");
  logger->set_pattern(kPattern);

  auto kad_logger = libp2p::common::createLogger("kad");
  kad_logger->set_pattern(kPattern);
  if (kad_log_debug) {
    kad_logger->set_level(spdlog::level::debug);
  }
}

int main(int argc, char* argv[]) {
  size_t hosts_count = 6;
  bool kad_log_debug = false;
  if (argc > 1) hosts_count = atoi(argv[1]);
  if (argc > 2) kad_log_debug = (atoi(argv[2]) != 0);

  setupLoggers(kad_log_debug);

  auto io = libp2p::kad_example::createIOContext();
  LowResTimerAsioImpl timer(*io, 200);

  Hosts hosts(hosts_count);

  hosts.listen(io);
  hosts.connect();
  hosts.findPeers(timer);

  boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
  signals.async_wait(
    [&io](const boost::system::error_code&, int) { io->stop(); }
  );
  io->run();

  hosts.checkPeers();
}
