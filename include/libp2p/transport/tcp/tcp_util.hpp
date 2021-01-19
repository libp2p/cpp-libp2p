/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_TCP_CONNECTION_UTIL_HPP
#define LIBP2P_TCP_CONNECTION_UTIL_HPP

#include <sstream>
#include <system_error>  // for std::errc

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <gsl/span>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/outcome/outcome.hpp>

namespace libp2p::transport::detail {
  template <typename T>
  inline outcome::result<multi::Multiaddress> makeAddress(T &&endpoint) {
    try {
      auto address = endpoint.address();
      auto port = endpoint.port();

      // TODO(warchant): PRE-181 refactor to use builder instead
      std::ostringstream s;
      if (address.is_v4()) {
        s << "/ip4/" << address.to_v4().to_string();
      } else {
        s << "/ip6/" << address.to_v6().to_string();
      }

      s << "/tcp/" << port;

      return multi::Multiaddress::create(s.str());
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }

  inline auto makeBuffer(gsl::span<uint8_t> s) {
    return boost::asio::buffer(s.data(), s.size());
  }

  inline auto makeBuffer(gsl::span<uint8_t> s, size_t size) {
    return boost::asio::buffer(s.data(), size);
  }

  inline auto makeBuffer(gsl::span<const uint8_t> s) {
    return boost::asio::buffer(s.data(), s.size());
  }

  inline auto makeBuffer(gsl::span<const uint8_t> s, size_t size) {
    return boost::asio::buffer(s.data(), size);
  }

  inline bool supportsIpTcp(const multi::Multiaddress &ma) {
    using P = multi::Protocol::Code;
    return (ma.hasProtocol(P::IP4) || ma.hasProtocol(P::IP6)
            || ma.hasProtocol(P::DNS4) || ma.hasProtocol(P::DNS6)
            || ma.hasProtocol(P::DNS))
        && ma.hasProtocol(P::TCP);

    // TODO(xDimon): Support 'DNSADDR' addresses.
    //  Issue: https://github.com/libp2p/cpp-libp2p/issues/97
  }

  inline auto getFirstProtocol(const multi::Multiaddress &ma) {
    return ma.getProtocolsWithValues().front().first.code;
  }

  // Obtain host and port strings from provided address
  inline std::pair<std::string, std::string> getHostAndTcpPort(
      const multi::Multiaddress &address) {
    auto v = address.getProtocolsWithValues();

    // get host
    auto it = v.begin();
    auto host = it->second;

    // get port
    it++;
    BOOST_ASSERT(it->first.code == multi::Protocol::Code::TCP);
    auto port = it->second;

    return {host, port};
  }

  inline outcome::result<boost::asio::ip::tcp::endpoint> makeEndpoint(
      const multi::Multiaddress &ma) {
    using P = multi::Protocol::Code;
    using namespace boost::asio;  // NOLINT

    try {
      auto v = ma.getProtocolsWithValues();
      auto it = v.begin();
      if (!(it->first.code == P::IP4 || it->first.code == P::IP6)) {
        return std::errc::address_family_not_supported;
      }

      auto addr = ip::make_address(it->second);
      ++it;

      if (it->first.code != P::TCP) {
        return std::errc::address_family_not_supported;
      }

      auto port = boost::lexical_cast<uint16_t>(it->second);

      return ip::tcp::endpoint{addr, port};
    } catch (const boost::system::system_error &e) {
      return e.code();
    } catch (const boost::bad_lexical_cast & /* ignored */) {
      return multi::Multiaddress::Error::INVALID_PROTOCOL_VALUE;
    }
  }
}  // namespace libp2p::transport::detail

#endif  // LIBP2P_TCP_CONNECTION_UTIL_HPP
