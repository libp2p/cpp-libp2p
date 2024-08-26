#include <audi/kusama.hpp>
#include <audi/replay.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/injector/kademlia_injector.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/protocol/identify.hpp>
#include <libp2p/protocol/ping.hpp>

template <typename T>
using P = std::shared_ptr<T>;
using audi::Key32;

void log() {
  std::string yaml = R"(
sinks:
 - name: console
   type: console
   color: true
groups:
 - name: main
   sink: console
   level: info
   children:
     - name: libp2p
)";
  auto log = std::make_shared<soralog::LoggingSystem>(
      std::make_shared<soralog::ConfiguratorFromYAML>(
          std::make_shared<libp2p::log::Configurator>(), yaml));
  auto r = log->configure();
  if (not r.message.empty()) {
    fmt::println("{} {}", r.has_error ? "E" : "W", r.message);
  }
  if (r.has_error) {
    exit(EXIT_FAILURE);
  }
  libp2p::log::setLoggingSystem(log);
}

std::vector<Key32> audi_txt(const std::string &path) {
  std::ifstream file{path};
  auto unhex32 = [&]() -> std::optional<Key32> {
    std::string s;
    file >> s;
    if (not file.good()) {
      return std::nullopt;
    }
    return qtils::unhex0x<Key32>(s).value();
  };
  std::vector<Key32> keys;
  while (auto id = unhex32()) {
    auto id_hash = unhex32().value();
    if (id_hash != libp2p::crypto::sha256(*id).value()) {
      throw std::runtime_error{"id_hash != hash(id)"};
    }
    keys.emplace_back(id_hash);
  }
  return keys;
}

// TODO: reuse keypair
auto injector() {
  libp2p::protocol::PingConfig ping_cfg;
  using KadCfg = libp2p::protocol::kademlia::Config;
  KadCfg kad_cfg;
  kad_cfg.protocols = {kusama_protocol};
  kad_cfg.valueLookupsQuorum = 1;
  kad_cfg.requestConcurency = 1;
  kad_cfg.randomWalk.enabled = false;
  return libp2p::injector::makeHostInjector(
      libp2p::injector::useKeyPair(audi::replay_peer()),
      libp2p::injector::makeKademliaInjector(
          boost::di::bind<decltype(ping_cfg)>().to(ping_cfg),
          boost::di::bind<KadCfg>().to(
              std::make_shared<KadCfg>(kad_cfg))[boost::di::override]));
}
P<libp2p::protocol::kademlia::Kademlia> kad;
P<libp2p::Host> host;
P<boost::asio::io_context> io;
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
    auto ping = di.create<P<libp2p::protocol::Ping>>();
    auto id = di.create<P<libp2p::protocol::Identify>>();
    inject(host);
    host->start();
    id->start();
    kad->start();
    host->setProtocolHandler(
        {ping->getProtocolId()},
        [=](libp2p::StreamAndProtocol s) { ping->handle(s); });
    for (auto &s : kusama_bootstrap) {
      auto a = libp2p::Multiaddress::create(s).value();
      auto p = libp2p::PeerId::fromBase58(a.getPeerId().value()).value();
      kad->addPeer({p, {a}}, false);
    }
  }
};

std::vector<Key32> keys;
size_t key_i = 0;
uint64_t t1 = 0;

void audi_loop() {
  if (key_i >= keys.size()) {
    fmt::println("done");
    exit(0);
  }
  fmt::println("key {}", key_i);
  t1 = audi::now();
  struct Once {
    int n = 0;
    ~Once() {
      if (n != 1) {
        fmt::println("LostCallback");
      }
    }
  };
  std::ignore = kad->getValue(
      qtils::asVec(keys[key_i]),
      [once{std::make_shared<Once>()}](outcome::result<qtils::Bytes> r) {
        ++once->n;
        auto t2 = audi::now();
        auto dt = t2 - t1;
        t1 = t2;
        fmt::println("key {} {} {}s", key_i, (bool)r, dt / 1000);
        ++key_i;
        audi_loop();
      });
}

void print_peer_count() {
  auto connected = [&] {
    auto peers =
        host->getNetwork().getConnectionManager().getConnections().size();
    fmt::println("peers {}", peers);
  };
  static auto e1 =
      host->getBus()
          .getChannel<libp2p::event::network::OnNewConnectionChannel>()
          .subscribe([&](std::weak_ptr<libp2p::connection::CapableConnection>) {
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
  if (argc != 2) {
    throw std::runtime_error{"argv[1]"};
  }
  keys = audi_txt(argv[1]);
  if (auto _n = getenv("AUDI_N")) {
    size_t n = atoi(_n);
    if (keys.size() > n) {
      keys.resize(n);
    }
  }
  audi::replay_writer_env();
  Swarm swarm;
  audi_loop();
  print_peer_count();
  io->run();
}
