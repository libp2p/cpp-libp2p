/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/multiaddress_parser.hpp"

#include <gtest/gtest.h>
#include <libp2p/common/literals.hpp>
#include <libp2p/outcome/outcome.hpp>
#include "testutil/outcome.hpp"

using libp2p::multi::Multiaddress;
using libp2p::multi::Protocol;
using libp2p::transport::MultiaddressParser;
using namespace libp2p::common;

using ::testing::Test;
using ::testing::TestWithParam;

struct TransportParserTest : public TestWithParam<Multiaddress> {};

/**
 * @given Transport parser and a multiaddress
 * @when parse the address
 * @then the chosen protocols by parser are the protocols of the multiaddress
 */
TEST_P(TransportParserTest, ParseSuccessfully) {
  auto r_ok = MultiaddressParser::parse(GetParam());
  std::vector<Protocol::Code> proto_codes;
  auto multiaddr_protos = GetParam().getProtocols();
  auto callback = [](Protocol const &proto) { return proto.code; };
  std::transform(multiaddr_protos.begin(), multiaddr_protos.end(),
                 std::back_inserter(proto_codes), callback);
  ASSERT_EQ(r_ok.value().chosen_protos, proto_codes);
}

auto addresses = {"/ip4/127.0.0.1/tcp/5050"_multiaddr};
INSTANTIATE_TEST_CASE_P(TestSupported, TransportParserTest,
                        ::testing::ValuesIn(addresses));

/**
 * @given Transport parser and a multiaddress
 * @when parse the address
 * @then the parse result variant contains information corresponding to the
 * content of the multiaddress
 */
TEST_F(TransportParserTest, CorrectAlternative) {
  auto r4 = MultiaddressParser::parse("/ip4/127.0.0.1/tcp/5050"_multiaddr);
  auto r6 = MultiaddressParser::parse(
      "/ip6/2001:db8:85a3:8d3:1319:8a2e:370:7348/tcp/443"_multiaddr);

  using Ip4Tcp = std::pair<MultiaddressParser::Ip4Address, uint16_t>;
  using Ip6Tcp = std::pair<MultiaddressParser::Ip6Address, uint16_t>;

  class ParserVisitor {
   public:
    /// IP/TCP address
    bool operator()(const Ip4Tcp &ip_tcp) {
      return ip_tcp.first.to_string() == "127.0.0.1" && ip_tcp.second == 5050;
    }
    bool operator()(const Ip6Tcp &ip_tcp) {
      return ip_tcp.first.to_string() == "2001:db8:85a3:8d3:1319:8a2e:370:7348"
          && ip_tcp.second == 443;
    }
  } visitor;

  ASSERT_TRUE(boost::apply_visitor(visitor, r4.value().data));
  ASSERT_TRUE(boost::apply_visitor(visitor, r6.value().data));
}
