/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/io_context.hpp>

#include <libp2p/peer/peer_info.hpp>

namespace libp2p::protocol::example::utility {

  /// Creates loggers "gossip", "debug", "gossip-example"
  /// returns "gossip-example" logger
  /// log level: 'd' for debug, 'i' for info, 'w' for warning, 'e' for error
  void setupLoggers(char level);

  /// Helper bytes->string
  std::string toString(const std::vector<uint8_t> &buf);

  /// Helper string->bytes
  std::vector<uint8_t> fromString(const std::string &s);

  /// Formats peer id as a short string
  std::string formatPeerId(const std::vector<uint8_t> &bytes);

  /// Returns the 1st local ipv4 as string
  std::string getLocalIP(boost::asio::io_context &io);

  /// Parses listen address and peer id given as uri
  boost::optional<libp2p::peer::PeerInfo> str2peerInfo(const std::string &str);

}  // namespace libp2p::protocol::example::utility
