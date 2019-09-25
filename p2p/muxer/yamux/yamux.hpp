/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_YAMUX_IMPL_HPP
#define KAGOME_YAMUX_IMPL_HPP

#include "muxer/muxed_connection_config.hpp"
#include "muxer/muxer_adaptor.hpp"

namespace libp2p::muxer {
  class Yamux : public MuxerAdaptor {
   public:
    ~Yamux() override = default;

    /**
     * Create a muxer with Yamux protocol
     * @param config of muxers to be created over the connections
     */
    explicit Yamux(
        MuxedConnectionConfig config);

    // NOLINTNEXTLINE(modernize-use-nodiscard)
    peer::Protocol getProtocolId() const noexcept override;

    void muxConnection(std::shared_ptr<connection::SecureConnection> conn,
                       CapConnCallbackFunc cb) const override;

   private:
    MuxedConnectionConfig config_;
  };
}  // namespace libp2p::muxer

#endif  // KAGOME_YAMUX_IMPL_HPP
