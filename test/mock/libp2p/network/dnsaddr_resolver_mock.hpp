/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TEST_MOCK_LIBP2P_NETWORK_DNSADDR_RESOLVER_MOCK_HPP
#define LIBP2P_TEST_MOCK_LIBP2P_NETWORK_DNSADDR_RESOLVER_MOCK_HPP

#include <libp2p/network/dnsaddr_resolver.hpp>

#include <gmock/gmock.h>

namespace libp2p::network {

  struct DnsaddrResolverMock : public DnsaddrResolver {
    MOCK_METHOD2(load, void(multi::Multiaddress, AddressesCallback));
  };
}  // namespace libp2p::network

#endif  // LIBP2P_TEST_MOCK_LIBP2P_NETWORK_DNSADDR_RESOLVER_MOCK_HPP
