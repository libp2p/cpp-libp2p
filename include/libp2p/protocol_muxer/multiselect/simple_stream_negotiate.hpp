/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_MUXER_SIMPLE_STREAM_NEGOTIATE_HPP
#define LIBP2P_PROTOCOL_MUXER_SIMPLE_STREAM_NEGOTIATE_HPP

#include <libp2p/connection/stream.hpp>
#include <libp2p/peer/protocol.hpp>

namespace libp2p::protocol_muxer::multiselect {

  /// Implements simple (Yes/No) negotiation of a single protocol on a fresh
  /// outbound stream
  void simpleStreamNegotiateImpl(
      const std::shared_ptr<connection::Stream> &stream,
      const peer::Protocol &protocol_id,
      std::function<void(outcome::result<std::shared_ptr<connection::Stream>>)>
          cb);

}  // namespace libp2p::protocol_muxer::multiselect

#endif  // LIBP2P_PROTOCOL_MUXER_SIMPLE_STREAM_NEGOTIATE_HPP
