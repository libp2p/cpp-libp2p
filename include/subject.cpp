#include <libp2p/injector/host_injector.hpp>
#include <libp2p/injector/kademlia_injector.hpp>
#include <libp2p/protocol/identify.hpp>
#include <libp2p/protocol/ping.hpp>
#include <qtils/unhex.hpp>
#include <soralog.hpp>
#include <time.hpp>

using libp2p::Multiaddress;
using libp2p::PeerId;
template <typename T>
using P = std::shared_ptr<T>;

uint32_t i_local = 0;
auto get_key(uint32_t i) {
  std::array<std::string_view, 2> keys{
      "f8dfdb0f1103d9fb2905204ac32529d5f148761c4321b2865b0a40e15be75f57",
      "96c891b8726cb18c781aefc082dbafcb827e16c8f18f22d461e83eabd618e780",
  };
  libp2p::crypto::ed25519::Keypair k;
  qtils::unhex(k.private_key, keys[i]).value();
  libp2p::crypto::ed25519::Ed25519ProviderImpl ed;
  k.public_key = ed.derive(k.private_key).value();
  using libp2p::crypto::Key;
  return libp2p::crypto::KeyPair{
      libp2p::crypto::PublicKey{{
          Key::Type::Ed25519,
          qtils::asVec(k.public_key),
      }},
      libp2p::crypto::PrivateKey{{
          Key::Type::Ed25519,
          qtils::asVec(k.private_key),
      }},
  };
}
auto get_peer(uint32_t i) {
  auto k = get_key(i);
  libp2p::crypto::marshaller::KeyMarshallerImpl m{nullptr};
  return PeerId::fromPublicKey(m.marshal(k.publicKey).value()).value();
}
auto get_addr(uint32_t i) {
  return libp2p::Multiaddress::create(
             fmt::format("/ip4/127.0.0.1/tcp/{}", 10000 + i))
      .value();
}

auto injector() {
  libp2p::protocol::PingConfig ping_cfg;
  using KadCfg = libp2p::protocol::kademlia::Config;
  KadCfg kad_cfg;
  kad_cfg.protocols = {"/dot/kad"};
  kad_cfg.valueLookupsQuorum = 1;
  kad_cfg.requestConcurency = 1;
  kad_cfg.randomWalk.enabled = false;
  return libp2p::injector::makeHostInjector(
      libp2p::injector::makeKademliaInjector(
          libp2p::injector::useKeyPair(get_key(i_local)),
          boost::di::bind<decltype(ping_cfg)>().to(ping_cfg),
          boost::di::bind<KadCfg>().to(
              std::make_shared<KadCfg>(kad_cfg))[boost::di::override]));
}
P<libp2p::protocol::kademlia::Kademlia> kad;
P<libp2p::Host> host;
P<libp2p::basic::Scheduler> scheduler;
P<boost::asio::io_context> io;
P<libp2p::protocol::Identify> id;
P<libp2p::protocol::Ping> ping;
struct Swarm {
  decltype(injector()) di = injector();
  template <typename T>
  void inject(T &t) {
    t = di.template create<T>();
  }
  Swarm() {
    log();
    inject(kad);
    inject(host);
    inject(io);
    inject(scheduler);
    inject(id);
    inject(ping);
    inject(host);
    host->start();
    id->start();
    kad->start();
    host->setProtocolHandler(
        {ping->getProtocolId()},
        [=](libp2p::StreamAndProtocol s) { ping->handle(s); });
  }
};

void print_peer_count() {
  auto connected = [&] {
    auto peers =
        host->getNetwork().getConnectionManager().getConnections().size();
    fmt::println("peers {}", peers);
  };
  static auto e1 =
      host->getBus()
          .getChannel<libp2p::event::network::OnNewConnectionChannel>()
          .subscribe(
              [&](std::weak_ptr<libp2p::connection::CapableConnection> wc) {
                if (auto c = wc.lock()) {
                  fmt::println("{} connected {} {}",
                               time(),
                               c->isInitiator(),
                               c->remotePeer().value().toBase58());
                }
                connected();
              });
  static auto e2 =
      host->getBus()
          .getChannel<libp2p::event::network::OnPeerDisconnectedChannel>()
          .subscribe([&](const libp2p::PeerId &) { connected(); });
}

int main(int argc, char **argv) {
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);
  assert(argc == 2);
  i_local = atoi(argv[1]);
  Swarm swarm;
  print_peer_count();
  host->listen(get_addr(i_local)).value();
  id->onIdentifyReceived([](const PeerId &peer) {
    fmt::println("{} id {}", time(), peer.toBase58());
    kad->addPeer(
        {
            peer,
            host->getPeerRepository()
                .getAddressRepository()
                .getAddresses(peer)
                .value(),
        },
        false,
        true);
  });
  if (i_local == 1) {
    uint32_t i_other = 0;
    host->connect({get_peer(i_other), {get_addr(i_other)}});
  }
  io->run();
}
