/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utility.hpp"

#include <boost/asio.hpp>

namespace libp2p::protocol::example::utility {

  std::string toString(const std::vector<uint8_t> &buf) {
    // NOLINTNEXTLINE
    return std::string(reinterpret_cast<const char *>(buf.data()), buf.size());
  }

  std::vector<uint8_t> fromString(const std::string &s) {
    std::vector<uint8_t> ret{};
    auto sz = s.size();
    if (sz > 0) {
      ret.reserve(sz);
      ret.assign(s.begin(), s.end());
    }
    return ret;
  }

  std::string formatPeerId(const std::vector<uint8_t> &bytes) {
    auto res = peer::PeerId::fromBytes(bytes);
    return res ? res.value().toBase58().substr(46) : "???";
  }

  void setupLoggers(char level) {
    switch (level) {
      case 'e':
        libp2p::log::setLevelOfGroup("main", libp2p::log::Level::ERROR);
        break;
      case 'w':
        libp2p::log::setLevelOfGroup("main", libp2p::log::Level::WARN);
        break;
      case 'd':
        libp2p::log::setLevelOfGroup("main", libp2p::log::Level::DEBUG);
        break;
      case 't':
        libp2p::log::setLevelOfGroup("main", libp2p::log::Level::TRACE);
        break;
      default:
        break;
    }
  }

  std::string getLocalIP(boost::asio::io_context &io) {
    boost::asio::ip::tcp::resolver resolver(io);
    boost::asio::ip::tcp::resolver::query query(boost::asio::ip::host_name(),
                                                "");
    boost::system::error_code ec;
    boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query, ec);
    boost::asio::ip::tcp::resolver::iterator end;
    std::string addr("127.0.0.1");
    while (it != end) {
      auto ep = it->endpoint();
      if (ep.address().is_v4()) {
        addr = ep.address().to_string();
        break;
      }
      ++it;
    }
    return addr;
  }

  boost::optional<libp2p::peer::PeerInfo> str2peerInfo(const std::string &str) {
    auto server_ma_res = libp2p::multi::Multiaddress::create(str);
    if (!server_ma_res) {
      return boost::none;
    }
    auto server_ma = std::move(server_ma_res.value());

    auto server_peer_id_str = server_ma.getPeerId();
    if (!server_peer_id_str) {
      return boost::none;
    }

    auto server_peer_id_res = peer::PeerId::fromBase58(*server_peer_id_str);
    if (!server_peer_id_res) {
      return boost::none;
    }

    return peer::PeerInfo{server_peer_id_res.value(), {server_ma}};
  }

}  // namespace libp2p::protocol::example::utility
