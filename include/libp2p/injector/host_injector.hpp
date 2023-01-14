/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_HOST_INJECTOR_HPP
#define LIBP2P_HOST_INJECTOR_HPP

#include <libp2p/injector/network_injector.hpp>

// implementations
#include <libp2p/host/basic_host.hpp>
#include <libp2p/peer/address_repository/inmem_address_repository.hpp>
#include <libp2p/peer/impl/peer_repository_impl.hpp>
#include <libp2p/peer/key_repository/inmem_key_repository.hpp>
#include <libp2p/peer/protocol_repository/inmem_protocol_repository.hpp>

namespace libp2p::injector {

  template <typename InjectorConfig = BOOST_DI_CFG, typename... Ts>
  inline auto makeHostInjector(Ts &&...args) {
    using namespace boost;  // NOLINT

    // clang-format off
    return di::make_injector<InjectorConfig>(
        makeNetworkInjector<InjectorConfig>(),

        // repositories
        di::bind<peer::PeerRepository>.template to<peer::PeerRepositoryImpl>(),
        di::bind<peer::AddressRepository>.template to<peer::InmemAddressRepository>(),
        di::bind<peer::KeyRepository>.template to<peer::InmemKeyRepository>(),
        di::bind<peer::ProtocolRepository>.template to<peer::InmemProtocolRepository>(),

        di::bind<Host>.template to<host::BasicHost>(),

        // user-defined overrides...
        std::forward<decltype(args)>(args)...
    );
    // clang-format on
  }

}  // namespace libp2p::injector

#endif  // LIBP2P_HOST_INJECTOR_HPP
