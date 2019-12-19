/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/converter_utils.hpp"

#include <gtest/gtest.h>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/multi/multiaddress_protocol_list.hpp>
#include "testutil/outcome.hpp"

using libp2p::common::ByteArray;
using libp2p::common::unhex;
using libp2p::multi::converters::bytesToMultiaddrString;
using libp2p::multi::converters::multiaddrToBytes;

#define VALID_STR_TO_BYTES(addr, bytes)                       \
  {                                                           \
    EXPECT_OUTCOME_TRUE_NAME(r1, v1, multiaddrToBytes(addr)); \
    EXPECT_OUTCOME_TRUE_NAME(r2, v2, unhex(bytes));           \
    ASSERT_EQ(v1, v2);                                        \
  }

#define VALID_BYTES_TO_STR(addr, bytes)                                      \
  {                                                                          \
    EXPECT_OUTCOME_TRUE_NAME(r3, v1, unhex(bytes));                          \
    EXPECT_OUTCOME_TRUE_NAME(r4, v2, bytesToMultiaddrString(ByteArray{v1})); \
    ASSERT_EQ(v2, addr);                                                     \
  }

/**
 * @given a multiaddr
 * @when converting it to hex string representing multiaddr byte representation
 * @then if the supplied address was valid, a valid hex string is returned
 */
TEST(AddressConverter, StringToBytes) {
  VALID_STR_TO_BYTES("/ip4/1.2.3.4", "0401020304")
  VALID_STR_TO_BYTES("/ip4/0.0.0.0", "0400000000")
  VALID_STR_TO_BYTES("/ip6/2001:db8::ff00:42:8329/",
                     "2920010db8000000000000ff0000428329")
  VALID_STR_TO_BYTES("/ip6/::1/", "2900000000000000000000000000000001")
  VALID_STR_TO_BYTES("/udp/0", "91020000")
  VALID_STR_TO_BYTES("/tcp/0", "060000")
  VALID_STR_TO_BYTES("/udp/1234", "910204D2")
  VALID_STR_TO_BYTES("/tcp/1234", "0604D2")
  VALID_STR_TO_BYTES(
      "/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/tcp/1234",
      "A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C"
      "0B0604D2");
  VALID_STR_TO_BYTES("/ip4/127.0.0.1/udp/1234/", "047F000001910204D2")
  VALID_STR_TO_BYTES("/ip4/127.0.0.1/udp/0/", "047F00000191020000")
  VALID_STR_TO_BYTES("/ip4/127.0.0.1/tcp/1234/", "047F0000010604D2")
  VALID_STR_TO_BYTES(
      "/ip4/127.0.0.1/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/",
      "047F000001A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20D"
      "B76A68911C0B")
  VALID_STR_TO_BYTES(
      "/ip4/127.0.0.1/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/tcp/"
      "1234/",
      "047F000001A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20D"
      "B76A68911C0B0604D2")
}

/**
 * @given a byte array with its content representing a multiaddr
 * @when converting it to a multiaddr human-readable string
 * @then if the supplied byte sequence was valid, a valid multiaddr string is
 * returned
 */
TEST(AddressConverter, BytesToString) {
  VALID_BYTES_TO_STR("/ip4/1.2.3.4", "0401020304")
  VALID_BYTES_TO_STR("/ip4/0.0.0.0", "0400000000")
  VALID_BYTES_TO_STR("/ip6/2001:db8::ff00:42:8329",
                     "2920010db8000000000000ff0000428329")
  VALID_BYTES_TO_STR("/ip6/::1", "2900000000000000000000000000000001")
  VALID_BYTES_TO_STR("/udp/0", "91020000")
  VALID_BYTES_TO_STR("/tcp/0", "060000")
  VALID_BYTES_TO_STR("/udp/1234", "910204D2")
  VALID_BYTES_TO_STR("/tcp/1234", "0604D2")
  VALID_BYTES_TO_STR(
      "/p2p/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/tcp/1234",
      "A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C"
      "0B0604D2")
  VALID_BYTES_TO_STR("/ip4/127.0.0.1/udp/1234", "047F000001910204D2")
  VALID_BYTES_TO_STR("/ip4/127.0.0.1/udp/0", "047F00000191020000")
  VALID_BYTES_TO_STR("/ip4/127.0.0.1/tcp/1234/udp/0/udp/1234",
                     "047F0000010604D291020000910204D2")
  VALID_BYTES_TO_STR(
      "/ip4/127.0.0.1/p2p/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC",
      "047F000001A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20D"
      "B76A68911C0B")
  VALID_BYTES_TO_STR(
      "/ip4/127.0.0.1/p2p/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/tcp/"
      "1234",
      "047F000001A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20D"
      "B76A68911C0B0604D2")

  ASSERT_FALSE(multiaddrToBytes("/"));
  ASSERT_FALSE(multiaddrToBytes("/ip4/8.8.8.8//"));
  ASSERT_FALSE(multiaddrToBytes("/tcp/udp/435/535"));
  ASSERT_FALSE(multiaddrToBytes("/43434/tcp"));
}
