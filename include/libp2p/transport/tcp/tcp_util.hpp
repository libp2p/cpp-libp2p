/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <charconv>
#include <libp2p/boost/outcome.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <variant>

namespace libp2p::transport::detail {
  using P = multi::Protocol::Code;

  struct Dns {
    std::optional<bool> v4;
    std::string name;
  };
  using IpOrDns = std::variant<boost::asio::ip::address, Dns>;

  static outcome::result<IpOrDns> readIpOrDns(ProtoAddrVec::iterator &it,
                                              ProtoAddrVec::iterator end) {
    if (it == end) {
      return std::errc::protocol_not_supported;
    }
    auto &[p, v] = *it++;
    switch (p.code) {
      case P::IP4:
      case P::IP6: {
        boost::asio::ip::address ip;
        boost::system::error_code ec;
        switch (p.code) {
          case P::IP4:
            ip = boost::asio::ip::make_address_v4(v, ec);
            break;
          case P::IP6:
            ip = boost::asio::ip::make_address_v6(v, ec);
            break;
        }
        if (ec) {
          return ec;
        }
        return ip;
      }
      case P::DNS:
      case P::DNS4:
      case P::DNS6: {
        Dns dns{std::nullopt, v};
        switch (p.code) {
          case P::DNS4:
            dns.v4 = true;
            break;
          case P::DNS6:
            dns.v4 = false;
            break;
        }
        return dns;
      }
    }
    return std::errc::protocol_not_supported;
  }

  struct TcpOrUdp {
    IpOrDns ip;
    uint16_t port = 0;
    bool udp = false;

    template <typename T>
    outcome::result<T> as(bool udp) const {
      auto ip = std::get_if<boost::asio::ip::address>(&this->ip);
      if (this->udp != udp or not ip) {
        return std::errc::protocol_not_supported;
      }
      return T{*ip, port};
    }
    auto asTcp() const {
      return as<boost::asio::ip::tcp::endpoint>(false);
    }
    auto asUdp() const {
      return as<boost::asio::ip::udp::endpoint>(true);
    }
  };

  static outcome::result<TcpOrUdp> readTcpOrUdp(ProtoAddrVec::iterator &it,
                                                ProtoAddrVec::iterator end) {
    TcpOrUdp addr;
    BOOST_OUTCOME_TRY(addr.ip, readIpOrDns(it, end));
    if (it == end) {
      return std::errc::protocol_not_supported;
    }
    auto &[p, v] = *it++;
    switch (p.code) {
      case P::TCP:
        addr.udp = false;
        break;
      case P::UDP:
        addr.udp = true;
        break;
      default:
        return std::errc::protocol_not_supported;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto r = std::from_chars(v.data(), v.data() + v.size(), addr.port);
    if (r.ec != std::errc{}) {
      return make_error_code(r.ec);
    }
    return addr;
  }

  inline outcome::result<std::pair<TcpOrUdp, ProtoAddrVec>> asTcp(
      const Multiaddress &ma) {
    auto v = ma.getProtocolsWithValues();
    auto it = v.begin();
    OUTCOME_TRY(addr, readTcpOrUdp(it, v.end()));
    if (addr.udp) {
      return std::errc::protocol_not_supported;
    }
    auto end = std::find_if(
        it, v.end(), [](const auto &p) { return p.first.code == P::P2P; });
    return std::make_pair(addr, ProtoAddrVec{it, end});
  }

  inline outcome::result<TcpOrUdp> asQuic(const Multiaddress &ma) {
    auto v = ma.getProtocolsWithValues();
    auto it = v.begin();
    OUTCOME_TRY(addr, readTcpOrUdp(it, v.end()));
    if (not addr.udp) {
      return std::errc::protocol_not_supported;
    }
    if (it == v.end()) {
      return std::errc::protocol_not_supported;
    }
    if (it->first.code != P::QUIC_V1) {
      return std::errc::protocol_not_supported;
    }
    return addr;
  }

  template <typename T>
  void resolve(T &resolver, const TcpOrUdp &addr, auto &&cb, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero()) {
    if (auto ip = std::get_if<boost::asio::ip::address>(&addr.ip)) {
      return cb(T::results_type::create(
          typename T::endpoint_type{*ip, addr.port}, "", ""));
    }

    auto &dns = std::get<Dns>(addr.ip);

    // Create a timer for timeout if specified
    std::shared_ptr<boost::asio::deadline_timer> timer;
    if (timeout > std::chrono::milliseconds::zero()) {
      timer = std::make_shared<boost::asio::deadline_timer>(resolver.get_executor());
      timer->expires_from_now(boost::posix_time::milliseconds(timeout.count()));

      // Start the timeout timer
      timer->async_wait([&resolver, cb=cb](const boost::system::error_code &error) mutable {
        if (not error) {
          // Cancel the ongoing resolution operation
          resolver.cancel();
          // Timeout error
          cb(boost::system::error_code{boost::system::errc::timed_out,
                                      boost::system::generic_category()});
        }
      });
    }

    auto cb2 = [cb{std::forward<decltype(cb)>(cb)}, timer](
                   boost::system::error_code ec,
                   typename T::results_type r) mutable {
      // Cancel the timer if it exists
      if (timer) {
        timer->cancel();
      }

      if (ec) {
        return cb(ec);
      }
      cb(std::move(r));
    };

    if (dns.v4) {
      resolver.async_resolve(
          *dns.v4 ? T::protocol_type::v4() : T::protocol_type::v6(),
          dns.name,
          std::to_string(addr.port),
          std::move(cb2));
    } else {
      resolver.async_resolve(
          dns.name, std::to_string(addr.port), std::move(cb2));
    }
  }

  template <typename T>
  outcome::result<std::string> toMultiaddr(const T &endpoint) {
    constexpr auto tcp = std::is_same_v<T, boost::asio::ip::tcp::endpoint>;
    constexpr auto udp = std::is_same_v<T, boost::asio::ip::udp::endpoint>;
    static_assert(tcp or udp);
    auto ip = endpoint.address();
    OUTCOME_TRY(ip_str, boost_outcome::to_string(ip));
    return fmt::format("/{}/{}/{}/{}",
                       ip.is_v4() ? "ip4" : "ip6",
                       ip_str,
                       tcp ? "tcp" : "udp",
                       endpoint.port());
  }

  inline outcome::result<Multiaddress> makeAddress(
      const boost::asio::ip::tcp::endpoint &endpoint,
      const ProtoAddrVec &layers) {
    OUTCOME_TRY(s, toMultiaddr(endpoint));
    if (not layers.empty()) {
      auto &protocol = layers.at(0).first.code;
      if (protocol == P::WS) {
        s += "/ws";
      } else if (protocol == P::WSS) {
        s += "/wss";
      }
    }
    return Multiaddress::create(s);
  }

  inline outcome::result<Multiaddress> makeQuicAddr(
      const boost::asio::ip::udp::endpoint &endpoint) {
    OUTCOME_TRY(s, toMultiaddr(endpoint));
    s += "/quic-v1";
    return Multiaddress::create(s);
  }
}  // namespace libp2p::transport::detail
