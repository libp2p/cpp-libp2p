/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/converter_utils.hpp"

#include <gtest/gtest.h>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>
#include <libp2p/multi/multiaddress_protocol_list.hpp>
#include "testutil/outcome.hpp"

using libp2p::Bytes;
using libp2p::common::unhex;
using libp2p::multi::converters::bytesToMultiaddrString;
using libp2p::multi::converters::ConversionError;
using libp2p::multi::converters::multiaddrToBytes;

#define EXAMINE_STR_TO_BYTES(str_addr, hex_bytes)               \
  do {                                                          \
    ASSERT_OUTCOME_SUCCESS(actual, multiaddrToBytes(str_addr)); \
    ASSERT_OUTCOME_SUCCESS(expected, unhex(hex_bytes));         \
    ASSERT_EQ(actual, expected);                                \
  } while (false)

#define EXAMINE_BYTES_TO_STR(str_addr, hex_bytes)                  \
  do {                                                             \
    auto &expected = str_addr;                                     \
    ASSERT_OUTCOME_SUCCESS(bytes, unhex(hex_bytes));               \
    ASSERT_OUTCOME_SUCCESS(actual, bytesToMultiaddrString(bytes)); \
    ASSERT_EQ(actual, expected);                                   \
  } while (false)

/**
 * @given a multiaddr
 * @when converting it to hex string representing multiaddr byte representation
 * @then if the supplied address was valid, a valid hex string is returned
 */
TEST(AddressConverter, StringToBytes) {
  EXAMINE_STR_TO_BYTES("/ip4/1.2.3.4", "0401020304");
  EXAMINE_STR_TO_BYTES("/ip4/0.0.0.0", "0400000000");

  EXAMINE_STR_TO_BYTES("/ip6/2001:db8::ff00:42:8329/",
                       "2920010db8000000000000ff0000428329");
  EXAMINE_STR_TO_BYTES("/ip6/::1/", "2900000000000000000000000000000001");

  EXAMINE_STR_TO_BYTES("/tcp/0", "060000");
  EXAMINE_STR_TO_BYTES("/tcp/1234", "0604D2");

  EXAMINE_STR_TO_BYTES("/udp/0", "91020000");
  EXAMINE_STR_TO_BYTES("/udp/1234", "910204D2");

  EXAMINE_STR_TO_BYTES("/ws", "DD03");
  EXAMINE_STR_TO_BYTES("/wss", "DE03");

  EXAMINE_STR_TO_BYTES(
      "/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC",
      "A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4"
      "EC20DB76A68911C0B");
  EXAMINE_STR_TO_BYTES(
      "/p2p/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC",
      "A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4"
      "EC20DB76A68911C0B");

  EXAMINE_STR_TO_BYTES("/ip4/127.0.0.1/tcp/1234", "047F0000010604D2");
  EXAMINE_STR_TO_BYTES("/ip4/127.0.0.1/tcp/1234/ws", "047F0000010604D2DD03");

  EXAMINE_STR_TO_BYTES(
      "/ip4/127.0.0.1/tcp/1234/p2p/"
      "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/",
      "047F0000010604D2A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC"
      "4EC20DB76A68911C0B");

  EXAMINE_STR_TO_BYTES(
      "/ip4/127.0.0.1/tcp/1234/ws/p2p/"
      "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/",
      "047F0000010604D2DD03A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42"
      "BEEC4EC20DB76A68911C0B");
}

/**
 * @given a byte array with its content representing a multiaddr
 * @when converting it to a multiaddr human-readable string
 * @then if the supplied byte sequence was valid, a valid multiaddr string is
 * returned
 */
TEST(AddressConverter, BytesToString) {
  EXAMINE_BYTES_TO_STR("/ip4/1.2.3.4", "0401020304");
  EXAMINE_BYTES_TO_STR("/ip4/0.0.0.0", "0400000000");

  EXAMINE_BYTES_TO_STR("/ip6/2001:db8::ff00:42:8329",
                       "2920010db8000000000000ff0000428329");
  EXAMINE_BYTES_TO_STR("/ip6/::1", "2900000000000000000000000000000001");

  EXAMINE_BYTES_TO_STR("/tcp/0", "060000");
  EXAMINE_BYTES_TO_STR("/tcp/1234", "0604D2");

  EXAMINE_BYTES_TO_STR("/udp/0", "91020000");
  EXAMINE_BYTES_TO_STR("/udp/1234", "910204D2");

  EXAMINE_BYTES_TO_STR("/ws", "DD03");
  EXAMINE_BYTES_TO_STR("/wss", "DE03");

  EXAMINE_BYTES_TO_STR(
      "/p2p/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC",
      "A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4"
      "EC20DB76A68911C0B");

  EXAMINE_BYTES_TO_STR("/ip4/127.0.0.1/tcp/1234", "047F0000010604D2");
  EXAMINE_BYTES_TO_STR("/ip4/127.0.0.1/tcp/1234/ws", "047F0000010604D2DD03");

  EXAMINE_BYTES_TO_STR(
      "/ip4/127.0.0.1/tcp/1234/p2p/"
      "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC",
      "047F0000010604D2A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC"
      "4EC20DB76A68911C0B");

  EXAMINE_BYTES_TO_STR(
      "/ip4/127.0.0.1/tcp/1234/ws/p2p/"
      "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC",
      "047F0000010604D2DD03A503221220D52EBB89D85B02A284948203A62FF28389C57C9F42"
      "BEEC4EC20DB76A68911C0B");
}

TEST(AddressConverter, InvalidAddresses) {
  ASSERT_OUTCOME_ERROR(multiaddrToBytes("ip4/127.0.0.1"),
                       ConversionError::ADDRESS_DOES_NOT_BEGIN_WITH_SLASH);
  ASSERT_OUTCOME_ERROR(multiaddrToBytes("/"), ConversionError::EMPTY_PROTOCOL);
  ASSERT_OUTCOME_ERROR(multiaddrToBytes("/ip4/8.8.8.8//"),
                       ConversionError::EMPTY_PROTOCOL);
  ASSERT_OUTCOME_ERROR(multiaddrToBytes("/fake"),
                       ConversionError::NO_SUCH_PROTOCOL);
  ASSERT_OUTCOME_ERROR(multiaddrToBytes("/80/tcp"),
                       ConversionError::NO_SUCH_PROTOCOL);
  ASSERT_OUTCOME_ERROR(multiaddrToBytes("/ip4/127.0.0.1/tcp"),
                       ConversionError::EMPTY_ADDRESS);
  ASSERT_OUTCOME_ERROR(multiaddrToBytes("/ip4/254.255.256.257/"),
                       ConversionError::INVALID_ADDRESS);
  ASSERT_OUTCOME_ERROR(multiaddrToBytes("/tcp/77777"),
                       ConversionError::INVALID_ADDRESS);
  ASSERT_OUTCOME_ERROR(multiaddrToBytes("/tcp/udp"),
                       ConversionError::INVALID_ADDRESS);
}
