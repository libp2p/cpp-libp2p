/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOLLIST_HPP
#define LIBP2P_PROTOCOLLIST_HPP

#include <array>
#include <functional>
#include <map>
#include <string_view>

namespace libp2p::multi {

  /**
   * Contains some data about a network protocol, e. g. its name and code
   */
  struct Protocol {
   public:
    /**
     * Denotes that the size of the protocol is variable
     */
    static const int kVarLen = -1;

    enum class Code : std::size_t {
      IP4 = 4,
      TCP = 6,
      UDP = 273,
      DCCP = 33,
      IP6 = 41,
      IP6_ZONE = 42,
      DNS = 53,
      DNS4 = 54,
      DNS6 = 55,
      DNS_ADDR = 56,
      SCTP = 132,
      UDT = 301,
      UTP = 302,
      UNIX = 400,
      P2P = 421,
      ONION = 444,
      ONION3 = 445,
      GARLIC64 = 446,
      QUIC = 460,
      HTTP = 480,
      HTTPS = 443,
      WS = 477,
      WSS = 478,
      P2P_WEBSOCKET_STAR = 479,
      P2P_STARDUST = 277,
      P2P_WEBRTC_STAR = 275,
      P2P_WEBRTC_DIRECT = 276,
      P2P_CIRCUIT = 290,
    };

    constexpr bool operator==(const Protocol &p) const {
      return code == p.code;
    }

    const Code code;
    const ssize_t size;
    const std::string_view name;
  };

  /**
   * Contains a list of protocols and some accessor methods for it
   */
  class ProtocolList {
   public:
    /**
     * The total number of known protocols
     */
    static const std::size_t kProtocolsNum = 28;

    /**
     * Returns a protocol with the corresponding name if it exists, or nullptr
     * otherwise
     */
    static constexpr auto get(std::string_view name) -> Protocol const * {
      if(name == "ipfs") {
        name = "p2p";  // IPFS is a legacy name, P2P is the preferred one
      }
      for (Protocol const &protocol : protocols_) {
        if (protocol.name == name) {
          return &protocol;
        }
      }
      return nullptr;
    }

    /**
     * Returns a protocol with the corresponding code if it exists, or nullptr
     * otherwise
     */
    static constexpr auto get(Protocol::Code code) -> Protocol const * {
      for (Protocol const &protocol : protocols_) {
        if (protocol.code == code) {
          return &protocol;
        }
      }
      return nullptr;
    }

    /**
     * Return the list of all known protocols
     * @return
     */
    static constexpr auto getProtocols()
        -> const std::array<Protocol, kProtocolsNum> & {
      return protocols_;
    }

   private:
    /**
     * The list of known protocols
     */
    static constexpr const std::array<Protocol, kProtocolsNum> protocols_ = {
        Protocol{Protocol::Code::IP4, 32, "ip4"},
        {Protocol::Code::TCP, 16, "tcp"},
        {Protocol::Code::UDP, 16, "udp"},
        {Protocol::Code::DCCP, 16, "dccp"},
        {Protocol::Code::IP6, 128, "ip6"},
        {Protocol::Code::IP6_ZONE, Protocol::kVarLen, "ip6zone"},
        {Protocol::Code::DNS, Protocol::kVarLen, "dns"},
        {Protocol::Code::DNS4, Protocol::kVarLen, "dns4"},
        {Protocol::Code::DNS6, Protocol::kVarLen, "dns6"},
        {Protocol::Code::DNS_ADDR, Protocol::kVarLen, "dnsaddr"},
        {Protocol::Code::SCTP, 16, "sctp"},
        {Protocol::Code::UDT, 0, "udt"},
        {Protocol::Code::UTP, 0, "utp"},
        {Protocol::Code::UNIX, Protocol::kVarLen, "unix"},
        {Protocol::Code::P2P, Protocol::kVarLen, "p2p"},
        // Also P2P protocol may have a legacy name "ipfs"
        {Protocol::Code::ONION, 96, "onion"},
        {Protocol::Code::ONION3, 296, "onion3"},
        {Protocol::Code::GARLIC64, Protocol::kVarLen, "garlic64"},
        {Protocol::Code::QUIC, 0, "quic"},
        {Protocol::Code::HTTP, 0, "http"},
        {Protocol::Code::HTTPS, 0, "https"},
        {Protocol::Code::WS, 0, "ws"},
        {Protocol::Code::WSS, 0, "wss"},
        {Protocol::Code::P2P_WEBSOCKET_STAR, 0, "p2p-websocket-star"},
        {Protocol::Code::P2P_STARDUST, 0, "p2p-stardust"},
        {Protocol::Code::P2P_WEBRTC_STAR, 0, "p2p-webrtc-star"},
        {Protocol::Code::P2P_WEBRTC_DIRECT, 0, "p2p-webrtc-direct"},
        {Protocol::Code::P2P_CIRCUIT, 0, "p2p-circuit"},
    };
  };

}  // namespace libp2p::multi
#endif  // LIBP2P_PROTOCOLLIST_HPP
