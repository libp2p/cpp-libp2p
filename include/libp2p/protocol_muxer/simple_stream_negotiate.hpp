/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_MUXER_SIMPLE_STREAM_NEGOTIATE_HPP
#define LIBP2P_PROTOCOL_MUXER_SIMPLE_STREAM_NEGOTIATE_HPP

#include <memory>
#include <type_traits>

#include <libp2p/basic/readwriter.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/peer/protocol.hpp>

namespace libp2p::protocol_muxer::multiselect {

  /// Implements simple (Yes/No) negotiation of a single protocol on a fresh
  /// outbound stream
  void simpleStreamNegotiateImpl(
      const std::shared_ptr<basic::ReadWriter> &stream,
      const peer::Protocol &protocol_id,
      std::function<void(outcome::result<void>)> cb);

  /// Simple outbound stream negotiate wrapper
  template <typename S>
  void simpleStreamNegotiate(
      std::shared_ptr<S> stream, const peer::Protocol &protocol_id,
      std::function<void(outcome::result<std::shared_ptr<S>>)> cb) {
    static_assert(std::is_base_of_v<basic::ReadWriter, S>,
                  "ReadWriter descendants required here");

    assert(!protocol_id.empty());
    assert(cb);

    simpleStreamNegotiateImpl(
        stream, protocol_id,
        [cb = std::move(cb), stream](outcome::result<void> r) {
          return r ? cb(stream) : cb(r.error());
        });
  }

}  // namespace libp2p::protocol_muxer::multiselect

#endif  // LIBP2P_PROTOCOL_MUXER_SIMPLE_STREAM_NEGOTIATE_HPP
