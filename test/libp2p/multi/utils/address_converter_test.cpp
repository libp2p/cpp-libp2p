/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/multi/converters/converter_utils.hpp>
#include <libp2p/multi/multiaddress_protocol_list.hpp>
#include "testutil/outcome.hpp"

using libp2p::common::ByteArray;
using libp2p::multi::ProtocolList;
using libp2p::multi::converters::addressToHex;

/**
 * @given A string with an ip4 address
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, Ip4AddressToBytes) {
  auto bytes = addressToHex(*ProtocolList::get("ip4"), "127.0.0.1").value();
  ASSERT_EQ(bytes, "7F000001");

  bytes = addressToHex(*ProtocolList::get("ip4"), "0.0.0.1").value();
  ASSERT_EQ(bytes, "00000001");

  bytes = addressToHex(*ProtocolList::get("ip4"), "0.0.0.0").value();
  ASSERT_EQ(bytes, "00000000");

  auto res = addressToHex(*ProtocolList::get("ip4"), "0.0.1");
  ASSERT_FALSE(res);

  res = addressToHex(*ProtocolList::get("ip4"), "0.0.0.1.");
  ASSERT_FALSE(res);
}

/**
 * @given A string with an ip6 address
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, Ip6AddressToBytes) {
  EXPECT_OUTCOME_TRUE(bytes,
                      addressToHex(*ProtocolList::get("ip6"),
                                   "2001:0db8:0000:0000:0000:ff00:0042:8329"));
  ASSERT_EQ(bytes, "20010db8000000000000ff0000428329");

  EXPECT_OUTCOME_TRUE(
      res, addressToHex(*ProtocolList::get("ip6"), "2001:db8::ff00:42:8329"));
  ASSERT_EQ(res, "20010db8000000000000ff0000428329");

  EXPECT_OUTCOME_FALSE_1(addressToHex(*ProtocolList::get("ip6"), "0.0.0.1"));

  EXPECT_OUTCOME_TRUE(res2, addressToHex(*ProtocolList::get("ip6"), "::1"));
  ASSERT_EQ(res2, "00000000000000000000000000000001");
}

/**
 * @given A string with a tcp address (a port, actually)
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, TcpAddressToBytes) {
  auto p = ProtocolList::get(libp2p::multi::Protocol::Code::TCP);
  ASSERT_EQ("04D2", addressToHex(*p, "1234").value());
  ASSERT_EQ("0000", addressToHex(*p, "0").value());
  ASSERT_FALSE(addressToHex(*p, "34343430"));
  ASSERT_FALSE(addressToHex(*p, "3434fd"));
}

/**
 * @given A string with a udp address (a port, actually)
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, UdpAddressToBytes) {
  auto p = ProtocolList::get(libp2p::multi::Protocol::Code::UDP);
  ASSERT_EQ("04D2", addressToHex(*p, "1234").value());
  ASSERT_EQ("0000", addressToHex(*p, "0").value());
  ASSERT_FALSE(addressToHex(*p, "34343430"));
  ASSERT_FALSE(addressToHex(*p, "f3434"));
  ASSERT_FALSE(addressToHex(*p, " 34343 "));
}

/**
 * @given A string with an ipfs address (base58 encoded)
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, IpfsAddressToBytes) {
  auto p = ProtocolList::get(libp2p::multi::Protocol::Code::P2P);
  ASSERT_EQ(
      "221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B",
      addressToHex(*p, "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC")
          .value());
  ASSERT_FALSE(
      addressToHex(*p, "QmcgpsyWgH81Il8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC"));
}
