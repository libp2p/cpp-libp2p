#include "../src/protocol/kademlia/kad2/kad2.hpp"
#include "factory.hpp"
#include <cassert>
#include <map>

int main() {
  using namespace libp2p::kad2;
  using namespace libp2p::kad_example;

  auto log = libp2p::common::createLogger("kad");
  log->set_pattern("%L %T.%e %v");
  log->set_level(spdlog::level::debug);

  static const std::string listen_to = "/ip4/127.0.0.1/tcp/2222";
  auto ma = libp2p::multi::Multiaddress::create(listen_to);
  assert(ma);

  auto io = libp2p::kad_example::createIOContext();
  std::shared_ptr<libp2p::Host> host;
  RoutingTablePtr table;
  createHostAndRoutingTable(host, table);
  assert(host && table);

  std::shared_ptr<Kad> kad_server = std::make_shared<KadImpl>(std::make_unique<HostAccessImpl>(*host), table);
  kad_server->start(true);

  io->post([&] {
    auto listen_res = host->listen(ma.value());
    assert(listen_res);
    host->start();
  });

  auto peer_id_str = host->getId().toBase58();
  std::string connect_to = listen_to + "/ipfs/" + peer_id_str;

  auto peer_info = str2peerInfo(connect_to);
  assert(peer_info);

  Message msg = createFindNodeRequest(host->getId(), std::optional<libp2p::peer::PeerInfo>());

  auto client = std::make_shared<KadSingleQueryClient>();
  client->dial(*host, peer_info.value(), std::move(msg));

  /*
  host->newStream(
    peer_info.value(), kad_server->getProtocolId(),
    [&log](auto &&stream_res) {
      if (!stream_res) {
        log->error("Cannot connect to server: {}", stream_res.error().message());
        return;
      }
      log->info("connected");

    }
  );
   */

  /*

  std::vector<LowResTimer::Handle> timers;
  bool canceled = false;
  LowResTimerAsioImpl timer(*io, 100);
  for (int i=1; i<16; ++i) {
    for (int j=0; j<i; ++j) {
      timers.push_back(timer.create(i*100, [&, i, j] {
        log->info("timer ({}, {})", i, j);
        if (!canceled) {
          for (size_t x=3; x<timers.size(); x+=3) {
            timers[x]->cancel();
          }
          canceled = true;
        }
      } ));
    }
  }
   */

  boost::asio::signal_set signals(*io, SIGINT, SIGTERM);
  signals.async_wait(
    [&io](const boost::system::error_code&, int) { io->stop(); }
  );
  io->run();
}
