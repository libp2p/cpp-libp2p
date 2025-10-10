/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux.hpp>

#include <libp2p/log/logger.hpp>
#include <libp2p/muxer/yamux/yamuxed_connection.hpp>
#include <libp2p/muxer/yamux/hardware_tracker.hpp>

namespace libp2p::muxer {
  Yamux::Yamux(MuxedConnectionConfig config,
               std::shared_ptr<basic::Scheduler> scheduler,
               std::shared_ptr<network::ConnectionManager> cmgr)
      : config_{config}, scheduler_{std::move(scheduler)} {
    connection::HardwareSharedPtrTracker::getInstance().enable();
    assert(scheduler_);
    if (cmgr) {
      std::weak_ptr<network::ConnectionManager> w(cmgr);
      close_cb_ = [wptr{std::move(w)}](
                      const peer::PeerId &p,
                      const std::shared_ptr<connection::CapableConnection> &c) {
        auto mgr = wptr.lock();
        if (mgr) {
          mgr->onConnectionClosed(p, c);
        }
      };
    }
  }

  peer::ProtocolName Yamux::getProtocolId() const {
    return "/yamux/1.0.0";
  }

  void Yamux::muxConnection(std::shared_ptr<connection::SecureConnection> conn,
                            CapConnCallbackFunc cb) const {
    if (conn == nullptr || conn->isClosed()) {
      log::createLogger("Yamux")->error("dead connection passed to muxer");
      return cb(std::errc::not_connected);
    }
    if (auto res = conn->remotePeer(); res.has_error()) {
      log::createLogger("Yamux")->error(
          "inactive connection passed to muxer: {}", res.error());
      return cb(res.error());
    }
    
    auto yamux_connection = std::make_shared<connection::YamuxedConnection>(
        std::move(conn), scheduler_, close_cb_, config_);
    connection::trackNextYamuxedConnection(yamux_connection);
    
    cb(yamux_connection);
  }
}  // namespace libp2p::muxer
