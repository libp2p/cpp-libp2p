/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/multi/converters/conversion_error.hpp>
#include <libp2p/multi/converters/converter_utils.hpp>
#include <libp2p/multi/multiaddress_protocol_list.hpp>
#include "testutil/outcome.hpp"

using libp2p::Bytes;
using libp2p::common::unhex;
using libp2p::multi::Protocol;
using libp2p::multi::ProtocolList;
using libp2p::multi::converters::addressToBytes;
using libp2p::multi::converters::ConversionError;

#define EXAMINE_STR_TO_BYTES(str_addr, hex_bytes, should_be_success)      \
  do {                                                                    \
    if (should_be_success) {                                              \
      ASSERT_OUTCOME_SUCCESS(actual, addressToBytes(protocol, str_addr)); \
      ASSERT_OUTCOME_SUCCESS(expected, unhex(hex_bytes));                 \
      ASSERT_EQ(actual, expected);                                        \
    } else {                                                              \
      ASSERT_OUTCOME_ERROR(addressToBytes(protocol, str_addr),            \
                           ConversionError::INVALID_ADDRESS);             \
    }                                                                     \
  } while (false)

#define EXAMINE_BYTES_TO_STR(str_addr, hex_bytes)                  \
  do {                                                             \
    auto &expected = str_addr;                                     \
    ASSERT_OUTCOME_SUCCESS(bytes, unhex(hex_bytes));               \
    ASSERT_OUTCOME_SUCCESS(actual, bytesToMultiaddrString(bytes)); \
    ASSERT_EQ(actual, expected);                                   \
  } while (false)

/**
 * @given A string with an ip4 address
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, Ip4AddressToBytes) {
  const auto &protocol = *ProtocolList::get(libp2p::multi::Protocol::Code::IP4);
  EXAMINE_STR_TO_BYTES("127.0.0.1", "7F000001", true);
  EXAMINE_STR_TO_BYTES("0.0.0.1", "00000001", true);
  EXAMINE_STR_TO_BYTES("0.0.0.0", "00000000", true);
  EXAMINE_STR_TO_BYTES("127.0.0", "", false);
  EXAMINE_STR_TO_BYTES("", "", false);
  EXAMINE_STR_TO_BYTES("127.0.0.1.", "", false);
}

/**
 * @given A string with an ip6 address
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, Ip6AddressToBytes) {
  const auto &protocol = *ProtocolList::get(libp2p::multi::Protocol::Code::IP6);
  EXAMINE_STR_TO_BYTES("2001:0db8:0000:0000:0000:ff00:0042:8329",
                       "20010db8000000000000ff0000428329",
                       true);
  EXAMINE_STR_TO_BYTES(
      "2001:db8::ff00:42:8329", "20010db8000000000000ff0000428329", true);
  EXAMINE_STR_TO_BYTES("::1", "00000000000000000000000000000001", true);
  EXAMINE_STR_TO_BYTES("", "", false);
  EXAMINE_STR_TO_BYTES("::1::", "", false);
  EXAMINE_STR_TO_BYTES("127.0.0.1", "", false);
  EXAMINE_STR_TO_BYTES("invalid", "", false);
}

/**
 * @given A string with a tcp address (a port, actually)
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, TcpAddressToBytes) {
  const auto &protocol = *ProtocolList::get(libp2p::multi::Protocol::Code::TCP);
  EXAMINE_STR_TO_BYTES("0", "0000", true);
  EXAMINE_STR_TO_BYTES("1234", "04D2", true);
  EXAMINE_STR_TO_BYTES("65535", "FFFF", true);
  EXAMINE_STR_TO_BYTES("65536", "", false);
  EXAMINE_STR_TO_BYTES("-1", "", false);
  EXAMINE_STR_TO_BYTES("", "", false);
  EXAMINE_STR_TO_BYTES("invalid", "", false);
}

/**
 * @given A string with a udp address (a port, actually)
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, UdpAddressToBytes) {
  const auto &protocol = *ProtocolList::get(libp2p::multi::Protocol::Code::UDP);
  EXAMINE_STR_TO_BYTES("0", "0000", true);
  EXAMINE_STR_TO_BYTES("1234", "04D2", true);
  EXAMINE_STR_TO_BYTES("65535", "FFFF", true);
  EXAMINE_STR_TO_BYTES("65536", "", false);
  EXAMINE_STR_TO_BYTES("-1", "", false);
  EXAMINE_STR_TO_BYTES("", "", false);
  EXAMINE_STR_TO_BYTES("invalid", "", false);
}

/**
 * @given A string with an ipfs address (base58 encoded)
 * @when converting it to bytes representation
 * @then if the address was valid then valid byte sequence representing the
 * address is returned
 */
TEST(AddressConverter, IpfsAddressToBytes) {
  const auto &protocol = *ProtocolList::get(libp2p::multi::Protocol::Code::P2P);
  EXAMINE_STR_TO_BYTES(
      "QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC",
      "221220D52EBB89D85B02A284948203A62FF28389C57C9F42BEEC4EC20DB76A68911C0B",
      true);
  EXAMINE_STR_TO_BYTES("", "", false);
  EXAMINE_STR_TO_BYTES("invalid", "", false);
}
