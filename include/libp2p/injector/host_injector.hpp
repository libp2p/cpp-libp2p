/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_HOST_INJECTOR_HPP
#define LIBP2P_HOST_INJECTOR_HPP

#include <libp2p/injector/network_injector.hpp>

// implementations
#include <libp2p/host/basic_host.hpp>
#include <libp2p/log/configurator.hpp>
#include <libp2p/peer/address_repository/inmem_address_repository.hpp>
#include <libp2p/peer/impl/peer_repository_impl.hpp>
#include <libp2p/peer/key_repository/inmem_key_repository.hpp>
#include <libp2p/peer/protocol_repository/inmem_protocol_repository.hpp>

namespace libp2p::injector {

  namespace details {
    template <typename Injector>
    std::shared_ptr<libp2p::log::Configurator> get_logger_configurator(
        const Injector &) {
      static boost::optional<std::shared_ptr<libp2p::log::Configurator>>
          instance;
      if (instance.has_value()) {
        return *instance;
      }

      instance = std::make_shared<libp2p::log::Configurator>(R"(
# This is libp2p configuration part of logging system
# ------------- Begin of libp2p config --------------
groups:
  - name: libp2p
    level: off
    children:
      - name: muxer
        children:
          - name: mplex
          - name: yamux
      - name: crypto
      - name: security
        children:
          - name: plaintext
          - name: secio
          - name: noise
      - name: protocols
        children:
          - name: echo
          - name: identify
          - name: kademlia
            level: trace
      - name: storage
        children:
          - name: sqlite
      - name: utils
        children:
          - name: debug
          - name: ares
          - name: tls
          - name: listener_manager
# --------------- End of libp2p config ---------------)");

      return *instance;
    }
  }  // namespace details

  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  auto makeHostInjector(Ts &&... args) {
    using namespace boost;  // NOLINT

    // clang-format off
    return di::make_injector<InjectorConfig>(
//        soralog::injector::makeInjector<InjectorConfig>(),

        makeNetworkInjector<InjectorConfig>(),

        di::bind<Host>.template to<host::BasicHost>(),

        di::bind<muxer::MuxedConnectionConfig>.to(muxer::MuxedConnectionConfig()),

        // repositories
        di::bind<peer::PeerRepository>.template to<peer::PeerRepositoryImpl>(),
        di::bind<peer::AddressRepository>.template to<peer::InmemAddressRepository>(),
        di::bind<peer::KeyRepository>.template to<peer::InmemKeyRepository>(),
        di::bind<peer::ProtocolRepository>.template to<peer::InmemProtocolRepository>(),

        // configure logging system
        di::bind<log::Configurator>.to([](const auto &i){return details::get_logger_configurator(i);}),
        di::bind<soralog::Configurator>.template to<log::Configurator>()[di::override],

        // user-defined overrides...
        std::forward<decltype(args)>(args)...
    );
    // clang-format on
  }

}  // namespace libp2p::injector

#endif  // LIBP2P_HOST_INJECTOR_HPP
