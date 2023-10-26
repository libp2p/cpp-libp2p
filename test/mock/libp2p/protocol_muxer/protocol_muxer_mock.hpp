/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/protocol_muxer/protocol_muxer.hpp>

#include <gmock/gmock.h>

namespace libp2p::protocol_muxer {
  class ProtocolMuxerMock : public ProtocolMuxer {
   public:
    ~ProtocolMuxerMock() override = default;

    MOCK_METHOD5(selectOneOf,
                 void(std::span<const peer::ProtocolName> protocols,
                      std::shared_ptr<basic::ReadWriter> connection,
                      bool is_initiator,
                      bool negotiate_multiselect,
                      ProtocolHandlerFunc cb));

    MOCK_METHOD3(
        simpleStreamNegotiate,
        void(const std::shared_ptr<connection::Stream> &,
             const peer::ProtocolName &,
             std::function<
                 void(outcome::result<std::shared_ptr<connection::Stream>>)>));
  };
}  // namespace libp2p::protocol_muxer
