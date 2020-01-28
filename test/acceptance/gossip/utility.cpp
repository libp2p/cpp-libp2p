/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utility.hpp"

#include <boost/asio.hpp>

#include <libp2p/crypto/sha/sha256.hpp>
#include <libp2p/crypto/ed25519_provider/ed25519_provider_impl.hpp>
#include <libp2p/crypto/crypto_provider/crypto_provider_impl.hpp>
#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/crypto/key_validator/key_validator_impl.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>

namespace libp2p::protocol::gossip::example {

  std::string toString(const std::vector<uint8_t> &buf) {
    // NOLINTNEXTLINE
    return std::string(reinterpret_cast<const char *>(buf.data()), buf.size());
  }

  std::vector<uint8_t> fromString(const std::string &s) {
    std::vector<uint8_t> ret{};
    auto sz = s.size();
    if (sz > 0) {
      ret.reserve(sz);
      ret.assign(s.begin(), s.end());
    }
    return ret;
  }

  std::string formatPeerId(const std::vector<uint8_t> &bytes) {
    auto res = peer::PeerId::fromBytes(bytes);
    return res ? res.value().toBase58().substr(46) : "???";
  }

  libp2p::common::Logger setupLoggers(bool debug_mode) {
    static const char *kPattern = "%L %T.%e %v";

    auto gossip_logger = libp2p::common::createLogger("gossip");
    gossip_logger->set_pattern(kPattern);

    auto debug_logger = libp2p::common::createLogger("debug");
    debug_logger->set_pattern(kPattern);

    auto logger = libp2p::common::createLogger("gossip-example");
    logger->set_pattern(kPattern);

    if (debug_mode) {
      gossip_logger->set_level(spdlog::level::debug);
      debug_logger->set_level(spdlog::level::trace);
      logger->set_level(spdlog::level::debug);
    }

    return logger;
  }

  std::string getLocalIP(boost::asio::io_context &io) {
    boost::asio::ip::tcp::resolver resolver(io);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(),
                                                "");
    boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);
    boost::asio::ip::tcp::resolver::iterator end;
    std::string addr("127.0.0.1");
    while (it != end) {
      auto ep = it->endpoint();
      if (ep.address().is_v4()) {
        addr = ep.address().to_string();
        break;
      }
      ++it;
    }
    return addr;
  }

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

  boost::optional<EndpointInfo> makeEndpoint(const std::string &host,
                                             const std::string &port,
                                             const std::string &key) {
    auto private_key_raw = crypto::sha256(key);
    auto ed25519_provider =
        std::make_shared<crypto::ed25519::Ed25519ProviderImpl>();
    auto pubkey_res = ed25519_provider->derive(private_key_raw);
    if (!pubkey_res) {
      return boost::none;
    }
    crypto::PublicKey pubkey;
    pubkey.type = crypto::Key::Type::Ed25519;
    pubkey.data.assign(pubkey_res.value().begin(), pubkey_res.value().end());
    crypto::PrivateKey privkey;
    privkey.type = crypto::Key::Type::Ed25519;
    privkey.data.assign(private_key_raw.begin(), private_key_raw.end());

    auto csprng = std::make_shared<crypto::random::BoostRandomGenerator>();
    auto crypto_provider =
        std::make_shared<crypto::CryptoProviderImpl>(csprng, ed25519_provider);
    auto validator =
        std::make_shared<crypto::validator::KeyValidatorImpl>(crypto_provider);
    auto marshaller =
        std::make_shared<crypto::marshaller::KeyMarshallerImpl>(validator);

    auto protobuf_key_res = marshaller->marshal(pubkey);
    peer::PeerId peer_id =
        libp2p::peer::PeerId::fromPublicKey(protobuf_key_res.value()).value();

    boost::optional<multi::Multiaddress> addr;
    auto addr_res =
        multi::Multiaddress::create(fmt::format("/ip4/{}/tcp/{}", host, port));
    if (addr_res) {
      addr = addr_res.value();
    }

    return EndpointInfo{std::move(peer_id),
                        crypto::KeyPair{std::move(pubkey), std::move(privkey)},
                        std::move(addr)};
  }

}  // namespace libp2p::protocol::gossip::example
