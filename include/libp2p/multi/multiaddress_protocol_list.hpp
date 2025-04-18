/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <array>
#include <functional>
#include <map>
#include <string_view>
#include <unistd.h>

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

    // https://github.com/multiformats/multicodec
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
      QUIC_V1 = 461,
      HTTP = 480,
      HTTPS = 443,
      WS = 477,
      WSS = 478,
      P2P_WEBSOCKET_STAR = 479,
      P2P_STARDUST = 277,
      P2P_WEBRTC_STAR = 275,
      P2P_WEBRTC_DIRECT = 276,
      P2P_CIRCUIT = 290,
      // https://github.com/multiformats/rust-multiaddr/blob/3c7e813c3b1fdd4187a9ca9ff67e10af0e79231d/src/protocol.rs#L50-L53
      X_PARITY_WS = 4770,
      X_PARITY_WSS = 4780,
    // Range for private use: 0x300000 – 0x3FFFFF
    // Debug section
      _DUMMY_PROTO_1 = 0x3DEAD1,
      _DUMMY_PROTO_2 = 0x3DEAD2,
      _DUMMY_PROTO_3 = 0x3DEAD3,
      _DUMMY_PROTO_4 = 0x3DEAD4,
    };

    constexpr bool operator==(const Protocol &p) const {
      return code == p.code;
    }

    Code code;
    ssize_t size;
    std::string_view name;
  };

  /**
   * Contains a list of protocols and some accessor methods for it
   */
  class ProtocolList {
   public:
    /**
     * The total number of known protocols
     * (31 ordinal + 4 debug)
     */
    static constexpr size_t kProtocolsNum = 31 + 4;

    /**
     * Returns a protocol with the corresponding name if it exists, or nullptr
     * otherwise
     */
    static constexpr auto get(std::string_view name) -> const Protocol * {
      if (name == "ipfs") {
        name = "p2p";  // IPFS is a legacy name, P2P is the preferred one
      }
      for (const Protocol &protocol : protocols_) {
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
    static constexpr auto get(Protocol::Code code) -> const Protocol * {
      for (const Protocol &protocol : protocols_) {
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
        {Protocol::Code::QUIC_V1, 0, "quic-v1"},
        {Protocol::Code::HTTP, 0, "http"},
        {Protocol::Code::HTTPS, 0, "https"},
        {Protocol::Code::WS, 0, "ws"},
        {Protocol::Code::WSS, 0, "wss"},
        {Protocol::Code::P2P_WEBSOCKET_STAR, 0, "p2p-websocket-star"},
        {Protocol::Code::P2P_STARDUST, 0, "p2p-stardust"},
        {Protocol::Code::P2P_WEBRTC_STAR, 0, "p2p-webrtc-star"},
        {Protocol::Code::P2P_WEBRTC_DIRECT, 0, "p2p-webrtc-direct"},
        {Protocol::Code::P2P_CIRCUIT, 0, "p2p-circuit"},
        {Protocol::Code::X_PARITY_WS, Protocol::kVarLen, "x-parity-ws"},
        {Protocol::Code::X_PARITY_WSS, Protocol::kVarLen, "x-parity-wss"},
// Debug section
        {Protocol::Code::_DUMMY_PROTO_1, 0, "_dummy_proto_1"},
        {Protocol::Code::_DUMMY_PROTO_2, 0, "_dummy_proto_2"},
        {Protocol::Code::_DUMMY_PROTO_3, Protocol::kVarLen, "_dummy_proto_3"},
        {Protocol::Code::_DUMMY_PROTO_4, Protocol::kVarLen, "_dummy_proto_4"},

    };
  };

}  // namespace libp2p::multi
