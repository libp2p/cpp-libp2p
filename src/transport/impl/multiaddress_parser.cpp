/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/transport/impl/multiaddress_parser.hpp>

#include <boost/asio/ip/address.hpp>
#include <boost/lexical_cast.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::transport, MultiaddressParser::Error, e) {
  using E = libp2p::transport::MultiaddressParser::Error;
  switch (e) {
    case E::PROTOCOLS_UNSUPPORTED:
      return "This protocol is not supported by libp2p transport";
    case E::INVALID_ADDR_VALUE:
      return "Some of the values in the multiaddress are invalid";
  }
  return "Unknown error";
}

namespace libp2p::transport {

  using boost::lexical_cast;
  using boost::asio::ip::make_address;
  using multi::Protocol;

  // clang-format off
  const std::vector<std::vector<Protocol::Code>> MultiaddressParser::kSupportedProtocols {
    {multi::Protocol::Code::IP4, multi::Protocol::Code::TCP},
    {multi::Protocol::Code::IP6, multi::Protocol::Code::TCP}
  };
  // clang-format on

  outcome::result<MultiaddressParser::ParseResult> MultiaddressParser::parse(
      const multi::Multiaddress &address) {
    auto pvs = address.getProtocolsWithValues();

    int16_t entry_idx{};
    // find an entry in supported protocols list that is equal to the list of
    // protocols in the multiaddr
    auto entry = std::find_if(
        kSupportedProtocols.begin(), kSupportedProtocols.end(),
        [&pvs](auto const &entry) {
          return std::equal(entry.begin(), entry.end(), pvs.begin(), pvs.end(),
                            [](Protocol::Code entry_proto, auto &pair) {
                              return entry_proto == pair.first.code;
                            });
        });
    if (entry == kSupportedProtocols.end()) {
      return Error::PROTOCOLS_UNSUPPORTED;
    }
    // calculate the index of this entry in the supported protocols list
    entry_idx = std::distance(std::begin(kSupportedProtocols), entry);

    // decompose the values of the multiaddr to a data structure that can be
    // stored in ParseResult variant field, accroding to the sequence of
    // protocols in the entry
    switch (entry_idx) {
      // process ip[4 or 6]/tcp
      case 0:
      case 1: {
        auto it = pvs.begin();
        OUTCOME_TRY(addr, parseIp(it->second));
        it++;
        OUTCOME_TRY(port, parseTcp(it->second));
        if (entry_idx == 0) { // ipv4
          return ParseResult{*entry, std::make_pair(addr.to_v4(), port)};
        }
        // ipv6
        return ParseResult{*entry, std::make_pair(addr.to_v6(), port)};
      }
      default:
        return Error::PROTOCOLS_UNSUPPORTED;
    }
  }

  outcome::result<uint16_t> MultiaddressParser::parseTcp(
      std::string_view value) {
    try {
      return lexical_cast<uint16_t>(value);
    } catch (boost::bad_lexical_cast &) {
      return Error::INVALID_ADDR_VALUE;
    }
  }

  outcome::result<MultiaddressParser::IpAddress> MultiaddressParser::parseIp(
      std::string_view value) {
    boost::system::error_code ec;
    auto addr = make_address(value, ec);
    if (!ec) {
      return addr;
    }
    return Error::INVALID_ADDR_VALUE;
  }

}  // namespace libp2p::transport
