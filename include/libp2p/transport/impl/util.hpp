/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <sstream>
#include <system_error>  // for std::errc

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <span>

namespace libp2p::transport::detail
{
  template<typename T> // boost::asio::ip::tcp i.e.
  inline outcome::result<T> makeEndpointImpl( const multi::Multiaddress& ma )
  {
       using P = multi::Protocol::Code;
    using namespace boost::asio;  // NOLINT

    try {
      auto v = ma.getProtocolsWithValues();
      auto it = v.begin();
      if (it->first.code != P::IP4 && it->first.code != P::IP6) {
        return std::errc::address_family_not_supported;
      }

      auto addr = ip::make_address(it->second);
      ++it;

      if constexpr ( std::is_same_v<T,boost::asio::ip::tcp> )
      {  if(it->first.code != P::TCP) 
          return std::errc::address_family_not_supported;
      } else if (std::is_same_v<T, boost::asio::ip::udp> )
      {
        if( auto _code = it->first.code; _code != P::UDP || _code!=P::QUIC ) 
          return std::errc::address_family_not_supported;
      }
      

      auto port = boost::lexical_cast<uint16_t>(it->second);

      return T{addr, port};
    } catch (const boost::system::system_error &e) {
      return e.code();
    } catch (const boost::bad_lexical_cast & /* ignored */) {
      return multi::Multiaddress::Error::INVALID_PROTOCOL_VALUE;
    }
  }


  inline outcome::result<boost::asio::ip::udp::endpoint> makeUdpEndpoint(
      const multi::Multiaddress &ma) {
 return makeEndpointImpl<boost::asio::ip::udp::endpoint>( ma);
  }

  inline outcome::result<boost::asio::ip::tcp::endpoint> makeTcpEndpoint(
      const multi::Multiaddress &ma) {
 return makeEndpointImpl<boost::asio::ip::tcp::endpoint>( ma);
  }

  template <typename T>
  inline outcome::result<multi::Multiaddress> makeAddress(
      T &&endpoint,
      const std::vector<std::pair<multi::Protocol, std::string>> *layers =
          nullptr) {
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

      if constexpr( std::is_same_v<T,boost::asio::ip::tcp> )
      {  s << "/tcp/" << port;}
      else if ( std::is_same_v<T,boost::asio::ip::udp> )
      {
          s<<"/quic/" << port; // udp would be pointless because unreliable
      }

      if (layers != nullptr and not layers->empty()) {
        auto &protocol = layers->at(0).first.code;
        using P = multi::Protocol::Code;
        if (protocol == P::WS) {
          s << "/ws";
        } else if (protocol == P::WSS) {
          s << "/wss";
        }
      }

      return multi::Multiaddress::create(s.str());
    } catch (const boost::system::system_error &e) {
      return e.code();
    }
  }

  inline auto makeBuffer(BytesOut s) {
    return boost::asio::buffer(s.data(), s.size());
  }

  inline auto makeBuffer(BytesOut s, size_t size) {
    return boost::asio::buffer(s.data(), size);
  }

  inline auto makeBuffer(BytesIn s) {
    return boost::asio::buffer(s.data(), s.size());
  }

  inline auto makeBuffer(BytesIn s, size_t size) {
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

    inline bool supportsIpQuic(const multi::Multiaddress &ma) {
    using P = multi::Protocol::Code;
    return (ma.hasProtocol(P::IP4) || ma.hasProtocol(P::IP6)
            || ma.hasProtocol(P::DNS4) || ma.hasProtocol(P::DNS6)
            || ma.hasProtocol(P::DNS))
        && ma.hasProtocol(P::QUIC);

    // TODO(xDimon): Support 'DNSADDR' addresses.
    //  Issue: https://github.com/libp2p/cpp-libp2p/issues/97
  }

  inline auto getFirstProtocol(const multi::Multiaddress &ma) {
    return ma.getProtocolsWithValues().front().first.code;
  }

  // Obtain host and port strings from provided address
  inline std::pair<std::string, std::string> getHostAndPort(
      const multi::Multiaddress &address) {
    auto v = address.getProtocolsWithValues();

    // get host
    auto it = v.begin();
    auto host = it->second;

    // get port
    it++;
    BOOST_ASSERT(it->first.code == multi::Protocol::Code::TCP
    || it->first.code == multi::Protocol::Code::QUIC );
    auto port = it->second;

    return {host, port};
  }

  using ProtoAddrVec = std::vector<std::pair<multi::Protocol, std::string>>;

  // Obtain layers string from provided address
  inline ProtoAddrVec getLayers(const multi::Multiaddress &address) {
    auto v = address.getProtocolsWithValues();

    auto begin = std::next(v.begin(), 2);  // skip host and port

    auto end = std::find_if(begin, v.end(), [](const auto &p) {
      return p.first.code == multi::Protocol::Code::P2P;
    });

    return {begin, end};
  }

  
}  // namespace libp2p::transport::detail
