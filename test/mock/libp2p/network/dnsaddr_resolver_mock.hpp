/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/network/dnsaddr_resolver.hpp>

#include <gmock/gmock.h>

namespace libp2p::network {

  struct DnsaddrResolverMock : public DnsaddrResolver {
    MOCK_METHOD2(load, void(multi::Multiaddress, AddressesCallback));
  };
}  // namespace libp2p::network
