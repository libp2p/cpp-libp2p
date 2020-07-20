#include <libp2p/common/logger.hpp>
#include <libp2p/injector/host_injector.hpp>
#include <libp2p/multi/content_identifier_codec.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/protocol.hpp>
#include <libp2p/protocol/kademlia/impl/kad_impl.hpp>
#include <libp2p/protocol/kademlia/kad_injector.hpp>

#include <iostream>

using namespace libp2p;  // NOLINT

struct Config {
  std::string rendezvous_string;
  std::vector<multi::Multiaddress> bootstrap_peers;
  std::vector<multi::Multiaddress> listen_addresses;
  peer::Protocol protocol_id;
};

std::vector<multi::Multiaddress> defaultBootstrapPeers() {
  // clang-format off
  std::vector<std::string> string_mas = {
//      "/dnsaddr/bootstrap.libp2p.io/ipfs/QmNnooDu7bfjPFoTZYxMNLWUQJyrVwtbZg5gBMjTezGAJN",
//      "/dnsaddr/bootstrap.libp2p.io/ipfs/QmQCU2EcMqAqQPR2i9bChDtGNJchTbq5TbXJJ16u19uLTa",
//      "/dnsaddr/bootstrap.libp2p.io/ipfs/QmbLHAnMoJPWSCR5Zhtx6BHJX9KiKNN6tpvbUcqanj75Nb",
//      "/dnsaddr/bootstrap.libp2p.io/ipfs/QmcZf59bWwK5XFi76CZX8cbJ4BhTzzA3gU1ZjYZcYW3dwt",
      "/ip4/104.131.131.82/tcp/4001/ipfs/QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ",            // mars.i.ipfs.io
      "/ip4/104.236.179.241/tcp/4001/ipfs/QmSoLPppuBtQSGwKDZT2M73ULpjvfd3aZ6ha4oFGL1KrGM",           // pluto.i.ipfs.io
      "/ip4/128.199.219.111/tcp/4001/ipfs/QmSoLSafTMBsPKadTEgaXctDQVcqN88CNLHXMkTNwMKPnu",           // saturn.i.ipfs.io
      "/ip4/104.236.76.40/tcp/4001/ipfs/QmSoLV4Bbm51jM9C4gDYZQ9Cy3U6aXMJDAbzgu2fzaDs64",             // venus.i.ipfs.io
      "/ip4/178.62.158.247/tcp/4001/ipfs/QmSoLer265NRgSp2LA3dPaeykiS1J6DifTC88f5uVQKNAd",            // earth.i.ipfs.io
      "/ip6/2604:a880:1:20::203:d001/tcp/4001/ipfs/QmSoLPppuBtQSGwKDZT2M73ULpjvfd3aZ6ha4oFGL1KrGM",  // pluto.i.ipfs.io
      "/ip6/2400:6180:0:d0::151:6001/tcp/4001/ipfs/QmSoLSafTMBsPKadTEgaXctDQVcqN88CNLHXMkTNwMKPnu",  // saturn.i.ipfs.io
      "/ip6/2604:a880:800:10::4a:5001/tcp/4001/ipfs/QmSoLV4Bbm51jM9C4gDYZQ9Cy3U6aXMJDAbzgu2fzaDs64", // venus.i.ipfs.io
      "/ip6/2a03:b0c0:0:1010::23:1001/tcp/4001/ipfs/QmSoLer265NRgSp2LA3dPaeykiS1J6DifTC88f5uVQKNAd", // earth.i.ipfs.io
  };
  // clang-format on
  std::vector<multi::Multiaddress> result;
  result.reserve(string_mas.size());
  for (const auto &str_ma : string_mas) {
    result.push_back(multi::Multiaddress::create(str_ma).value());
  }
  return result;
}

void writeData(std::shared_ptr<connection::Stream> stream) {
  std::thread t([stream] {
    while (true) {
      if (stream->isClosed()) {
        break;
      }
      std::cout << "> ";
      std::string str;
      std::cin >> str;
      std::vector<uint8_t> buf{str.begin(), str.end()};
      stream->write(buf, buf.size(), [](const auto &res) {
        if (not res) {
          std::cerr << "Could not write: {}" << res.error().message();
        }
      });
    }
  });
  t.join();
}

void readData(std::shared_ptr<connection::Stream> stream) {
  std::vector<uint8_t> buf;
  buf.resize(4096);
  stream->read(
      buf, buf.size(), [buf, stream](const outcome::result<size_t> &size_res) {
        if (not size_res) {
          std::cerr << "Could not read: {}" << size_res.error().message();
          return;
        }
        std::cout << "Received: {}"
                  << std::string{buf.begin(), buf.end() + size_res.value()};
        readData(stream);
      });
}

void handlerStream(std::shared_ptr<connection::Stream> stream) {
  writeData(stream);
  readData(stream);
}

int main(int argc, char *argv[]) {
  auto logger = common::createLogger("rendezvous");

  Config config{.rendezvous_string = "meet me here",
                .bootstrap_peers = defaultBootstrapPeers(),
                .listen_addresses =
                    {
                        multi::Multiaddress::create(argv[1]).value(),
                    },
                .protocol_id = "/chat/1.1.0"};

  auto injector =
      injector::makeHostInjector(protocol::kademlia::makeKadInjector());
  std::shared_ptr<Host> host = injector.create<std::shared_ptr<Host>>();

  host->setProtocolHandler(config.protocol_id, handlerStream);

  std::shared_ptr<protocol::kademlia::Kad> kademliaDHT =
      injector.create<std::shared_ptr<protocol::kademlia::Kad>>();

  for (const auto &bootstrap_ma : config.bootstrap_peers) {
    auto bootstrap_peer_id_str = bootstrap_ma.getPeerId();
    if (not bootstrap_peer_id_str) {
      logger->error("invalid peer id in ma: {}",
                    bootstrap_ma.getStringAddress());
      return 1;
    }
    auto bootstrap_peer_id =
        libp2p::peer::PeerId::fromBase58(*bootstrap_peer_id_str);
    if (not bootstrap_peer_id) {
      logger->error("invalid peer id in ma: {}, error: {}",
                    bootstrap_ma.getStringAddress(),
                    bootstrap_peer_id.error().message());
      return 1;
    }
    auto peer_info =
        libp2p::peer::PeerInfo{bootstrap_peer_id.value(), {bootstrap_ma}};
    kademliaDHT->addPeer(peer_info, true);
    logger->info("Added bootstrap peer {}", bootstrap_peer_id_str.value());
    break;
  }

  // init
  auto context = injector.create<std::shared_ptr<boost::asio::io_context>>();
  context->post([host{std::move(host)}, config, logger, kademliaDHT] {
    for (const auto &listen_ma : config.listen_addresses) {
      if (auto res = host->listen(listen_ma); not res) {
        logger->error("Cannot listen address {}, reason: {}",
                      listen_ma.getStringAddress(), res.error().message());
      }
      logger->info("Server started listensing with address {}, peer id: {}",
                   listen_ma.getStringAddress(), host->getId().toBase58());
    }
    host->start();

    kademliaDHT->start(true);

    // Advertise
    protocol::kademlia::ContentAddress rendezvous_cad(config.rendezvous_string);
    kademliaDHT->putValue(
        rendezvous_cad,
        protocol::kademlia::Value{config.rendezvous_string.begin(),
                                  config.rendezvous_string.end()},
        [logger](const auto &res) {
          if (not res) {
            logger->error("Could not put value: {}", res.error().message());
          }
        });

    kademliaDHT->findPeer(
        peer::PeerId::fromBase58(
            "12D3KooWJ6NYZFrwNgQqVi4egmJTGF8XUM7mTxPtBAfXqnR9SwHj")
            .value(),
        [logger](const auto& peer, const auto &res) {
          logger->info("Found? {}", peer.toBase58());
        });

    //    auto peers = kademliaDHT->findProviders(rendezvous_cad);
    //    logger->info("Found {} providers", peers.size());

    // Find providers
    //     auto peerChan = kademliaDHT->findProviders()
  });

  // run the IO context
  try {
    context->run();
  } catch (const boost::system::error_code &ec) {
    std::cout << "Server cannot run: " << ec.message() << std::endl;
  } catch (...) {
    std::cout << "Unknown error happened" << std::endl;
  }
  return 0;
}
