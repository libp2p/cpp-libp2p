/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <memory>

#include <libp2p/common/literals.hpp>
#include <libp2p/network/connection_manager.hpp>
#include <libp2p/protocol/kademlia/impl/asio_scheduler_impl.hpp>
#include <libp2p/protocol/kademlia/impl/kad_impl.hpp>
#include <libp2p/protocol/kademlia/node_id.hpp>

#include "factory.hpp"

namespace libp2p::protocol::kademlia::example {

  static const uint16_t kPortBase = 40000;

  libp2p::common::Logger logger;

  libp2p::peer::PeerId genRandomPeerId(
      libp2p::crypto::CryptoProvider &gen,
      libp2p::crypto::marshaller::KeyMarshaller &marshaller) {
    auto keypair = gen.generateKeys(libp2p::crypto::Key::Type::Ed25519).value();
    return libp2p::peer::PeerId::fromPublicKey(
               marshaller.marshal(keypair.publicKey).value())
        .value();
  }

  const KademliaConfig &getConfig() {
    static KademliaConfig config = ([] {
      KademliaConfig c;
      c.randomWalk.delay = 5s;
      return c;
    })();
    return config;
  }

  struct Hosts {
    struct Host {
      size_t index;
      PerHostObjects o;
      std::shared_ptr<KadImpl> kad;
      std::string listen_to;
      std::string connect_to;
      boost::optional<libp2p::peer::PeerId> find_id;
      Scheduler::Handle htimer;
      Scheduler::Handle hbootstrap;
      bool found = false;
      bool verbose = true;
      bool request_sent = false;

      Host(size_t i, std::shared_ptr<Scheduler> sch, PerHostObjects obj)
          : index(i),
            o(std::move(obj))

      {
        kad = std::make_shared<KadImpl>(o.host, std::move(sch), o.routing_table,
                                        createDefaultValueStoreBackend(),
                                        getConfig());
      }

      void checkPeers() {
        auto peers = o.host->getPeerRepository().getPeers();
        logger->info("host {}: peers in repo: {}, found: {}", index,
                     peers.size(), found);
      }

      void listen(std::shared_ptr<boost::asio::io_context> &io) {
        if (listen_to.empty())
          return;
        io->post([this] { onListen(); });
      }

      void onListen() {
        auto ma = libp2p::multi::Multiaddress::create(listen_to).value();
        auto listen_res = o.host->listen(ma);
        if (!listen_res) {
          logger->error("Server {} cannot listen the given multiaddress: {}",
                        index, listen_res.error().message());
        }

        o.host->start();
        logger->info("server {} listening to: {} peerId={}", index,
                     ma.getStringAddress(),
                     o.host->getPeerInfo().id.toBase58());

        kad->start(true);
      }

      void connect() {
        if (connect_to.empty())
          return;
        auto pi = str2peerInfo(connect_to);
        kad->addPeer(std::move(pi.value()), true);
      }

      void findPeer(const libp2p::peer::PeerId &id) {
        find_id = id;
        htimer =
            kad->scheduler().schedule(20000, [this] { onFindPeerTimer(); });
        hbootstrap =
            kad->scheduler().schedule(100, [this] { onBootstrapTimer(); });
      }

      void onBootstrapTimer() {
        hbootstrap.reschedule(2000);
        if (!request_sent) {
          request_sent =
              kad->findPeer(genRandomPeerId(*o.key_gen, *o.key_marshaller),
                            [this](const libp2p::peer::PeerId &peer,
                                   Kad::FindPeerQueryResult res) {  // NOLINT
                              logger->info(
                                  "bootstrap return from findPeer, i={}, "
                                  "peer={} peers={} ({})",
                                  index, peer.toBase58(),
                                  res.closer_peers.size(), res.success);
                              request_sent = false;
                            });
          logger->info("bootstrap sent request, i={}, request_sent={}", index,
                       request_sent);
        } else {
          logger->info("bootstrap waiting for result, i={}", index);
        }
        o.host->getNetwork().getConnectionManager().collectGarbage();
      }

      void onFindPeerTimer() {
        logger->info("find peer timer callback, i={}", index);
        if (!found) {
          kad->findPeer(find_id.value(),
                        [this](const libp2p::peer::PeerId &peer,
                               Kad::FindPeerQueryResult res) {
                          onFindPeer(peer, std::move(res));
                        });
        }
      }

      void onFindPeer(const libp2p::peer::PeerId &peer,
                      Kad::FindPeerQueryResult res) {
        if (found)
          return;
        if (res.success) {
          found = true;
          logger->info("onFindPeer: i={}, res: success={}, peers={}", index,
                       res.success, res.closer_peers.size());
          return;
        }
        if (verbose)
          logger->info("onFindPeer: i={}, res: success={}, peers={}", index,
                       res.success, res.closer_peers.size());

        htimer = kad->scheduler().schedule(
            1000, [this, peers = std::move(res.closer_peers)] {
              kad->findPeer(find_id.value(),
                            [this](const libp2p::peer::PeerId &peer,
                                   Kad::FindPeerQueryResult res) {
                              onFindPeer(peer, std::move(res));
                            });

              if (!found && !peers.empty()) {
                kad->findPeer(find_id.value(), peers,
                              [this](const libp2p::peer::PeerId &peer,
                                     Kad::FindPeerQueryResult res) {
                                onFindPeer(peer, std::move(res));
                              });
              }
            });
      }
    };

    std::vector<Host> hosts;

    Hosts(size_t n, const std::shared_ptr<Scheduler> &sch) {
      hosts.reserve(n);

      for (size_t i = 0; i < n; ++i) {
        newHost(sch);
      }

      makeConnectTopologyCircle();
    }

    void newHost(const std::shared_ptr<Scheduler> &sch) {
      size_t index = hosts.size();
      PerHostObjects o;
      createPerHostObjects(o, getConfig());
      assert(o.host && o.routing_table);
      hosts.emplace_back(index, sch, std::move(o));
    }

    void makeConnectTopologyCircle() {
      for (auto &h : hosts) {
        h.listen_to = std::string("/ip4/127.0.0.1/tcp/")
            + std::to_string(kPortBase + h.index);
      }
      for (auto &h : hosts) {
        Host &server = hosts[(h.index > 0) ? h.index - 1 : hosts.size() - 1];
        if (!server.listen_to.empty()) {
          h.connect_to =
              server.listen_to + "/ipfs/" + server.o.host->getId().toBase58();
        }
      }
    }

    void listen(std::shared_ptr<boost::asio::io_context> &io) {
      for (auto &h : hosts) {
        h.listen(io);
      }
    }

    void connect() {
      for (auto &h : hosts) {
        h.connect();
      }
    }

    void findPeers() {
      for (auto &h : hosts) {
        auto half = hosts.size() / 2;
        auto index = h.index;
        if (index > half) {
          index -= half;
        } else {
          index += (half - 1);
        }
        Host &server = hosts[index];
        h.findPeer(server.o.host->getId());
      }
    }

    void checkPeers() {
      for (auto &h : hosts) {
        h.checkPeers();
      }
      hosts.clear();
    }
  };

  void setupLoggers(bool kad_log_debug) {
    static const char *kPattern = "%L %T.%e %v";

    logger = libp2p::common::createLogger("kad-example");
    logger->set_pattern(kPattern);

    auto kad_logger = libp2p::common::createLogger("kad");
    kad_logger->set_pattern(kPattern);
    if (kad_log_debug) {
      kad_logger->set_level(spdlog::level::debug);
    }

    auto debug_logger = libp2p::common::createLogger("debug");
    debug_logger->set_pattern(kPattern);
    debug_logger->set_level(spdlog::level::debug);
  }

}  //  namespace libp2p::protocol::kademlia::example

int main(int argc, char *argv[]) {
  namespace k = libp2p::protocol::kademlia;
  namespace x = k::example;
  try {
    size_t hosts_count = 6;
    bool kad_log_debug = false;
    if (argc > 1)
      hosts_count = atoi(argv[1]);  // NOLINT
    if (argc > 2)
      kad_log_debug = (atoi(argv[2]) != 0);  // NOLINT

    x::setupLoggers(kad_log_debug);

    auto io = x::createIOContext();
    auto scheduler = k::AsioSchedulerImpl::create(*io, 1000);

    x::Hosts hosts(hosts_count, scheduler);

    hosts.listen(io);
    hosts.connect();
    hosts.findPeers();

    boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
    signals.async_wait(
        [&io](const boost::system::error_code &, int) { io->stop(); });
    io->run();

    hosts.checkPeers();
  } catch (const std::exception &e) {
    if (x::logger) {
      x::logger->error("Exception: {}", e.what());
    }
  }
}
