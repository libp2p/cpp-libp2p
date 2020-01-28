/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_GOSSIP_EXAMPLE_UTILITY_HPP
#define LIBP2P_GOSSIP_EXAMPLE_UTILITY_HPP

#include <boost/asio/io_context.hpp>

#include <libp2p/peer/peer_info.hpp>

namespace libp2p::protocol::gossip::example {

  /// Creates loggers "gossip", "debug", "gossip-example"
  /// returns "gossip-example" logger
  libp2p::common::Logger setupLoggers(bool debug_mode);

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

  /// Helper structure for examples
  struct EndpointInfo {
    peer::PeerId peer_id;
    crypto::KeyPair keypair;
    boost::optional<multi::Multiaddress> address;
  };

  ///  Generates EndpointInfo using sha256(key) as private key
  boost::optional<EndpointInfo> makeEndpoint(const std::string &host,
                                             const std::string &port,
                                             const std::string &key);

}  // namespace libp2p::protocol::gossip::example

// wraps member fn via lambda
template <typename R, typename... Args, typename T>
auto bind_memfn(T *object, R (T::*fn)(Args...)) {
  return [object, fn](Args... args) {
    return (object->*fn)(std::forward<Args>(args)...);
  };
}
#define MEMFN_CALLBACK(M) \
  bind_memfn(this, &std::remove_pointer<decltype(this)>::type::M)

template <typename R, typename... Args, typename T, typename S>
auto bind_memfn_s(T *object, R (T::*fn)(const S &state, Args...),
                  const S &state) {
  return [object, fn, state = state](Args... args) {
    return (object->*fn)(state, std::forward<Args>(args)...);
  };
}
#define MEMFN_CALLBACK_S(M, S) \
  bind_memfn_s(this, &std::remove_pointer<decltype(this)>::type::M, S)

#endif  // LIBP2P_GOSSIP_EXAMPLE_UTILITY_HPP
