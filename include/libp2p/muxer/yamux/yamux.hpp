/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_YAMUX_IMPL_HPP
#define LIBP2P_YAMUX_IMPL_HPP

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/muxer/muxer_adaptor.hpp>
#include <libp2p/network/connection_manager.hpp>

namespace libp2p::muxer {
  class Yamux : public MuxerAdaptor {
   public:
    ~Yamux() override = default;

    /**
     * Create a muxer with Yamux protocol
     * @param config of muxers to be created over the connections
     * @param scheduler scheduler
     * @param cmgr connection manager. May be nullptr in tests, otherwise
     * close_cb_ is created using it
     */
    Yamux(MuxedConnectionConfig config,
          std::shared_ptr<basic::Scheduler> scheduler,
          std::shared_ptr<network::ConnectionManager> cmgr);

    peer::Protocol getProtocolId() const noexcept override;

    void muxConnection(std::shared_ptr<connection::SecureConnection> conn,
                       CapConnCallbackFunc cb) const override;

   private:
    MuxedConnectionConfig config_;
    std::shared_ptr<basic::Scheduler> scheduler_;
    connection::CapableConnection::ConnectionClosedCallback close_cb_;
  };
}  // namespace libp2p::muxer

#endif  // LIBP2P_YAMUX_IMPL_HPP
