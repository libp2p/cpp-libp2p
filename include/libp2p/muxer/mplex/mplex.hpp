/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/muxer/muxer_adaptor.hpp>

namespace libp2p::muxer {
  class Mplex : public MuxerAdaptor {
   public:
    explicit Mplex(MuxedConnectionConfig config);

    peer::ProtocolName getProtocolId() const noexcept override;

    void muxConnection(std::shared_ptr<connection::SecureConnection> conn,
                       CapConnCallbackFunc cb) const override;

   private:
    MuxedConnectionConfig config_;
  };
}  // namespace libp2p::muxer
