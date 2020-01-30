/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */


#include "factory.hpp"

#include <boost/di/extension/scopes/shared.hpp>

#include <libp2p/injector/host_injector.hpp>
#include <libp2p/protocol/kademlia/impl/routing_table_impl.hpp>

namespace libp2p::protocol::kademlia::example {

  boost::optional<libp2p::peer::PeerInfo> str2peerInfo(const std::string &str) {
    auto server_ma_res = libp2p::multi::Multiaddress::create(str);
    if (!server_ma_res) {
      fmt::print("unable to create server multiaddress: {}\n",
                 server_ma_res.error().message());
      return boost::none;
    }
    auto server_ma = std::move(server_ma_res.value());

    auto server_peer_id_str = server_ma.getPeerId();
    if (!server_peer_id_str) {
      fmt::print("unable to extract peer id from multiaddress\n");
      return boost::none;
    }

    auto server_peer_id_res = peer::PeerId::fromBase58(*server_peer_id_str);
    if (!server_peer_id_res) {
      fmt::print("Unable to decode peer id from base 58: {}\n",
                 server_peer_id_res.error().message());
      return boost::none;
    }

    return peer::PeerInfo{server_peer_id_res.value(), {server_ma}};
  }

  namespace {
    template <typename... Ts>
    auto makeInjector(Ts &&... args) {
      auto csprng = std::make_shared<crypto::random::BoostRandomGenerator>();
      auto ed25519_provider =
          std::make_shared<crypto::ed25519::Ed25519ProviderImpl>();
      auto rsa_provider = std::make_shared<crypto::rsa::RsaProviderImpl>();
      auto ecdsa_provider =
          std::make_shared<crypto::ecdsa::EcdsaProviderImpl>();
      auto secp256k1_provider =
          std::make_shared<crypto::secp256k1::Secp256k1ProviderImpl>();
      auto hmac_provider = std::make_shared<crypto::hmac::HmacProviderImpl>();

      std::shared_ptr<crypto::CryptoProvider> crypto_provider =
          std::make_shared<crypto::CryptoProviderImpl>(
              csprng, ed25519_provider, rsa_provider, ecdsa_provider,
              secp256k1_provider, hmac_provider);
      auto validator = std::make_shared<crypto::validator::KeyValidatorImpl>(
          crypto_provider);

      // assume no error here. otherwise... just blow up executable
      auto keypair =
          crypto_provider->generateKeys(crypto::Key::Type::Ed25519).value();

      return libp2p::injector::makeHostInjector<
          boost::di::extension::shared_config>(
          boost::di::bind<crypto::CryptoProvider>().to(
              crypto_provider)[boost::di::override],
          boost::di::bind<crypto::KeyPair>().template to(
              std::move(keypair))[boost::di::override],
          boost::di::bind<crypto::random::CSPRNG>().template to(
              std::move(csprng))[boost::di::override],
          boost::di::bind<crypto::marshaller::KeyMarshaller>()
              .template to<
                  crypto::marshaller::KeyMarshallerImpl>()[boost::di::override],
          boost::di::bind<crypto::validator::KeyValidator>().template to(
              std::move(validator))[boost::di::override],

          std::forward<decltype(args)>(args)...);
    }

  }  // namespace

  void createPerHostObjects(PerHostObjects &objects,
                            const KademliaConfig &conf) {
    auto injector = makeInjector(boost::di::bind<boost::asio::io_context>.to(
        createIOContext())[boost::di::override]);

    objects.host = injector.create<std::shared_ptr<libp2p::Host>>();
    objects.key_gen =
        injector.create<std::shared_ptr<libp2p::crypto::CryptoProvider>>();
    objects.key_marshaller = injector.create<
        std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller>>();
    objects.routing_table = std::make_shared<RoutingTableImpl>(
        injector.create<std::shared_ptr<peer::IdentityManager>>(),
        injector.create<std::shared_ptr<event::Bus>>(), conf);
  }

  std::shared_ptr<boost::asio::io_context> createIOContext() {
    static std::shared_ptr<boost::asio::io_context> c =
        std::make_shared<boost::asio::io_context>();
    return c;
  }

}  // namespace libp2p::protocol::kademlia::example
