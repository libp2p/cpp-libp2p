/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/multiaddress.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "libp2p/multi/multiaddress_protocol_list.hpp"

using libp2p::multi::Multiaddress;
using libp2p::multi::Protocol;
using libp2p::multi::ProtocolList;
using std::string_literals::operator""s;

using namespace kagome::common;

class MultiaddressTest : public ::testing::Test {
 public:
  const std::string_view valid_ip_udp = "/ip4/192.168.0.1/udp/228";
  const std::vector<uint8_t> valid_ip_udp_bytes = "04C0A80001910200E4"_unhex;
  const Buffer valid_id_udp_buffer{valid_ip_udp_bytes};

  const std::string_view invalid_addr = "/ip4/192.168.0.1/2";
  const std::vector<uint8_t> invalid_addr_bytes = "04C0A8000102"_unhex;
  const Buffer invalid_addr_buffer{invalid_addr_bytes};
};

/**
 * @given valid string address
 * @when creating a multiaddress from it
 * @then creation succeeds
 */
TEST_F(MultiaddressTest, CreateFromStringValid) {
  auto result = Multiaddress::create(valid_ip_udp);
  ASSERT_TRUE(result);
  auto &&address = result.value();
  ASSERT_EQ(address.getStringAddress(), valid_ip_udp);
  ASSERT_EQ(address.getBytesAddress(), valid_ip_udp_bytes);
}

/**
 * @given invalid string address
 * @when creating a multiaddress from it
 * @then creation fails
 */
TEST_F(MultiaddressTest, CreateFromStringInvalid) {
  auto result = Multiaddress::create(invalid_addr);
  ASSERT_FALSE(result);
}

/**
 * @given valid byte address
 * @when creating a multiaddress from it
 * @then creation succeeds
 */
TEST_F(MultiaddressTest, CreateFromBytesValid) {
  auto result = Multiaddress::create(valid_id_udp_buffer);
  ASSERT_TRUE(result);
  auto &&v = result.value();
  ASSERT_EQ(v.getStringAddress(), "/ip4/192.168.0.1/udp/228");
  ASSERT_EQ(v.getBytesAddress(), valid_ip_udp_bytes);
}

/**
 * @given invalid string address
 * @when creating a multiaddress from it
 * @then creation fails
 */
TEST_F(MultiaddressTest, CreateFromBytesInvalid) {
  auto result = Multiaddress::create(invalid_addr_buffer);
  ASSERT_FALSE(result);
}

/**
 * @given two valid multiaddresses
 * @when encapsulating one of them to another
 * @then encapsulated address' string and bytes representations are equal to
 * manually joined ones @and address, created from the joined string, is the
 * same, as the encapsulated one
 */
TEST_F(MultiaddressTest, Encapsulate) {
  auto address1 = "/ip4/192.168.0.1/udp/228"_multiaddr;
  auto address2 = "/p2p/mypeer"_multiaddr;

  auto joined_string_address =
      "/ip4/192.168.0.1/udp/228"s + "/p2p/mypeer";

  auto joined_byte_address = address1.getBytesAddress();
  joined_byte_address.putBuffer(address2.getBytesAddress());

  address1.encapsulate(address2);
  ASSERT_EQ(address1.getStringAddress(), joined_string_address);
  ASSERT_EQ(address1.getBytesAddress(), joined_byte_address);

  auto result = Multiaddress::create(joined_string_address);
  ASSERT_TRUE(result);
  ASSERT_EQ(result.value(), address1);
}

/**
 * @given valid multiaddress
 * @when decapsulating it with another address, which contains part of the
 * initial one
 * @then decapsulation is successful
 */
TEST_F(MultiaddressTest, DecapsulateValid) {
  auto initial_address = "/ip4/192.168.0.1/udp/228"_multiaddr;
  auto address_to_decapsulate = "/udp/228"_multiaddr;
  auto decapsulated_address = "/ip4/192.168.0.1"_multiaddr;
  ASSERT_TRUE(initial_address.decapsulate(address_to_decapsulate));
  ASSERT_EQ(initial_address, decapsulated_address);
}

/**
 * @given valid multiaddress
 * @when decapsulating it with another address, which does not ontain part of
 * the initial one
 * @then decapsulation fails
 */
TEST_F(MultiaddressTest, DecapsulateInvalid) {
  auto initial_address = "/ip4/192.168.0.1/udp/228"_multiaddr;
  auto address_to_decapsulate = "/p2p/mypeer"_multiaddr;

  ASSERT_FALSE(initial_address.decapsulate(address_to_decapsulate));
}

/**
 * @given valid multiaddress
 * @when getting its string representation
 * @then result is equal to the expected one
 */
TEST_F(MultiaddressTest, GetString) {
  auto address = "/ip4/192.168.0.1/udp/228"_multiaddr;
  ASSERT_EQ(address.getStringAddress(), "/ip4/192.168.0.1/udp/228");
}

/**
 * @given valid multiaddress
 * @when getting its byte representation
 * @then result is equal to the expected one
 */
TEST_F(MultiaddressTest, GetBytes) {
  EXPECT_OUTCOME_TRUE(address, Multiaddress::create(valid_ip_udp));
  ASSERT_EQ(address.getBytesAddress(), valid_ip_udp_bytes);
}

/**
 * @given valid multiaddress with IPFS part
 * @when getting peer id
 * @then it exists @and is equal to the expected one
 */
TEST_F(MultiaddressTest, GetPeerIdExists) {
  auto address = "/p2p/mypeer"_multiaddr;
  ASSERT_EQ(*address.getPeerId(), "mypeer");
}

/**
 * @given valid multiaddress without IPFS part
 * @when getting peer id
 * @then it does not exist
 */
TEST_F(MultiaddressTest, GetPeerIdNotExists) {
  auto address = "/ip4/192.168.0.1/udp/228"_multiaddr;
  ASSERT_FALSE(address.getPeerId());
}

/**
 * @given valid multiaddress
 * @when getting values for protocols, which exist in this multiaddress
 * @then they are returned
 */
TEST_F(MultiaddressTest, GetValueForProtocolValid) {
  auto address = "/ip4/192.168.0.1/udp/228/udp/432"_multiaddr;

  auto protocols = address.getValuesForProtocol(Protocol::Code::UDP);
  ASSERT_TRUE(!protocols.empty());
  ASSERT_EQ(protocols[0], "228");
  ASSERT_EQ(protocols[1], "432");
}

/**
 * @given valid multiaddress
 * @when getting values for protocols, which does not exist in this multiaddress
 * @then result is empty
 */
TEST_F(MultiaddressTest, GetValueForProtocolInvalid) {
  auto address = "/ip4/192.168.0.1/udp/228"_multiaddr;

  auto protocols = address.getValuesForProtocol(Protocol::Code::SCTP);
  ASSERT_TRUE(protocols.empty());
}

/**
 * @given valid multiaddress
 * @when getting protocols contained in the multiaddress
 * @then the list with all protocols which the multiaddress includes is obtained
 */
TEST_F(MultiaddressTest, GetProtocols) {
  auto address = "/ip4/192.168.0.1/udp/228"_multiaddr;
  ASSERT_THAT(address.getProtocols(),
              ::testing::ElementsAre(*ProtocolList::get("ip4"),
                                     *ProtocolList::get("udp")));

  address = "/p2p/mypeer"_multiaddr;
  ASSERT_THAT(address.getProtocols(),
              ::testing::ElementsAre(*ProtocolList::get("ipfs")));

  address = "/udp/322/ip4/127.0.0.1/udp/3232"_multiaddr;
  ASSERT_THAT(address.getProtocols(),
              ::testing::ElementsAre(*ProtocolList::get("udp"),
                                     *ProtocolList::get("ip4"),
                                     *ProtocolList::get("udp")));
}

/**
 * @given valid multiaddress
 * @when getting protocols contained in the multiaddress with their values
 * @then the list with all protocols and values which the multiaddress includes
 * is obtained
 */
TEST_F(MultiaddressTest, GetProtocolsWithValues) {
  auto address = "/ip4/192.168.0.1/udp/228"_multiaddr;
  ASSERT_THAT(address.getProtocolsWithValues(),
              ::testing::ElementsAre(
                  std::make_pair(*ProtocolList::get("ip4"), "192.168.0.1"),
                  std::make_pair(*ProtocolList::get("udp"), "228")));

  address = "/p2p/mypeer"_multiaddr;
  ASSERT_THAT(address.getProtocolsWithValues(),
              ::testing::ElementsAre(
                  std::make_pair(*ProtocolList::get("ipfs"), "mypeer")));

  address = "/udp/322/ip4/127.0.0.1/udp/3232"_multiaddr;
  ASSERT_THAT(address.getProtocolsWithValues(),
              ::testing::ElementsAre(
                  std::make_pair(*ProtocolList::get("udp"), "322"),
                  std::make_pair(*ProtocolList::get("ip4"), "127.0.0.1"),
                  std::make_pair(*ProtocolList::get("udp"), "3232")));
}
