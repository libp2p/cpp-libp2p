/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/identify/observed_addresses.hpp"

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>

using namespace libp2p;
using namespace protocol;
using namespace common;

class ObservedAddressesTest : public testing::Test {
 public:
  void SetUp() override {
    // for the purpose of testing, add some addresses to our object; in order
    // for the address to be "activated" 4 different observers must report about
    // it
    observed_addresses_.add(observed_ma1, local_ma1, observer_ma1, true);
    observed_addresses_.add(observed_ma1, local_ma1, observer_ma2, true);
    observed_addresses_.add(observed_ma1, local_ma1, observer_ma3, true);
    observed_addresses_.add(observed_ma1, local_ma1, observer_ma4, true);
    observed_addresses_.add(observed_ma2, local_ma2, observer_ma1, true);
    observed_addresses_.add(observed_ma2, local_ma2, observer_ma2, true);
    observed_addresses_.add(observed_ma2, local_ma2, observer_ma3, true);
  }

  ObservedAddresses observed_addresses_;

  multi::Multiaddress local_ma1 = "/ip4/92.134.23.14/tcp/225"_multiaddr,
                      local_ma2 = "/ip4/123.251.78.90/udp/228"_multiaddr,
                      observer_ma1 = "/ip4/123.251.78.91/udp/228"_multiaddr,
                      observer_ma2 = "/ip4/123.251.78.92/udp/228"_multiaddr,
                      observer_ma3 = "/ip4/123.251.78.93/udp/228"_multiaddr,
                      observer_ma4 = "/ip4/123.251.78.94/udp/228"_multiaddr,
                      observed_ma1 = "/ip4/123.251.78.96/udp/228"_multiaddr,
                      observed_ma2 = "/ip4/123.251.78.97/udp/228"_multiaddr;
};

/**
 * @given observed addresses object with some addresses inside
 * @when retrieving addresses for some local address
 * @then corresponding addresses are returned
 */
TEST_F(ObservedAddressesTest, GetAddressesFor) {
  auto addresses1 = observed_addresses_.getAddressesFor(local_ma1);
  ASSERT_EQ(addresses1.size(), 1);
  ASSERT_EQ(addresses1[0], observed_ma1);

  auto addresses2 = observed_addresses_.getAddressesFor(local_ma2);
  ASSERT_TRUE(addresses2.empty());
}

/**
 * @given observed addresses object with some addresses inside
 * @when retrieving all addresses
 * @then all addresses are returned
 */
TEST_F(ObservedAddressesTest, GetAllAddresses) {
  observed_addresses_.add(observed_ma2, local_ma2, observer_ma4, true);

  auto addresses = observed_addresses_.getAllAddresses();
  ASSERT_EQ(addresses.size(), 2);
  ASSERT_NE(std::find(addresses.begin(), addresses.end(), observed_ma1),
            addresses.end());
  ASSERT_NE(std::find(addresses.begin(), addresses.end(), observed_ma2),
            addresses.end());
}
