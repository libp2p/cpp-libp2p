/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTISELECT_COMMON_HPP
#define LIBP2P_MULTISELECT_COMMON_HPP

#include <string_view>

#include <boost/container/small_vector.hpp>

namespace libp2p::protocol_muxer::multiselect {

  /// Current protocol version
  static constexpr std::string_view kProtocolId = "/multistream/1.0.0";

  /// Message size limited by protocol
  static constexpr size_t kMaxMessageSize = 65535;

  /// Max varint size needed to hold kMaxMessageSize
  static constexpr size_t kMaxVarintSize = 3;

  /// New line character
  static constexpr uint8_t kNewLine = 0x0A;

  /// Special message N/A
  static constexpr std::string_view kNA("na");

  /// Multiselect protocol message, deflated
  struct Message {
    enum Type {
      kInvalidMessage,
      kRightProtocolVersion,
      kWrongProtocolVersion,
      kLSMessage,
      kNAMessage,
      kProtocolName,
    };

    Type type = kInvalidMessage;
    std::string_view content;
  };

  /// Vector that holds most of protocol messages w/o dynamic alloc
  using MsgBuf = boost::container::small_vector<uint8_t, 80>;

}  // namespace libp2p::protocol_muxer::multiselect

#endif  // LIBP2P_MULTISELECT_COMMON_HPP
