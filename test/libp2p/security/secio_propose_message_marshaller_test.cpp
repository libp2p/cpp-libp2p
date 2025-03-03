/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <libp2p/security/secio/propose_message_marshaller_impl.hpp>
#include <qtils/test/outcome.hpp>

using namespace libp2p::security::secio;

class ProposeMessageMarshallerTest : public ::testing::Test {
 protected:
  ProposeMessageMarshallerImpl marshaller;
};

/**
 * @given a SECIO propose message
 * @when the message is marshalled
 * @then the result of its unmarshalling equals to the source message
 */
TEST_F(ProposeMessageMarshallerTest, BasicCase) {
  ProposeMessage source{.rand = {1, 2, 3, 4, 5},
                        .pubkey = {6, 7, 8, 9, 10},
                        .exchanges = "think",
                        .ciphers = "of the",
                        .hashes = "rapture"};
  ASSERT_OUTCOME_SUCCESS(bytes, marshaller.marshal(source));
  ASSERT_OUTCOME_SUCCESS(derived, marshaller.unmarshal(bytes));
  ASSERT_EQ(source.rand, derived.rand);
  ASSERT_EQ(source.pubkey, derived.pubkey);
  ASSERT_EQ(source.exchanges, derived.exchanges);
  ASSERT_EQ(source.ciphers, derived.ciphers);
  ASSERT_EQ(source.hashes, derived.hashes);
}
