/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux.hpp>

#include <libp2p/log/logger.hpp>
#include <libp2p/muxer/yamux/yamuxed_connection.hpp>

namespace libp2p::muxer {
  Yamux::Yamux(MuxedConnectionConfig config,
               std::shared_ptr<basic::Scheduler> scheduler,
               std::shared_ptr<network::ConnectionManager> cmgr)
      : config_{config}, scheduler_{std::move(scheduler)}, cmgr_{cmgr} {
    assert(scheduler_);
    if (cmgr_) {
      std::weak_ptr<network::ConnectionManager> w(cmgr_);
      close_cb_ = [wptr{std::move(w)}](
                      const peer::PeerId &p,
                      const std::weak_ptr<connection::CapableConnection> &c) {
        auto mgr = wptr.lock();
        auto conn = c.lock();
        if (mgr && conn) {
          mgr->onConnectionClosed(p, conn);
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

    // Get the remote peer ID
    auto peer_id_res = conn->remotePeer();
    if (peer_id_res.has_error()) {
      log::createLogger("Yamux")->error(
          "inactive connection passed to muxer: {}", peer_id_res.error());
      return cb(peer_id_res.error());
    }

    const auto &peer_id = peer_id_res.value();

    // Check if a connection to this peer already exists
    if (cmgr_) {
      auto existing_conn = cmgr_->getBestConnectionForPeer(peer_id);
      if (existing_conn && !existing_conn->isClosed()) {
        log::createLogger("Yamux")->debug(
            "connection to peer {} already exists, reusing",
            peer_id.toBase58());
        return cb(existing_conn);
      }
    }

    log::createLogger("Yamux")->debug("muxing new connection to peer {}",
                                      peer_id.toBase58());
    cb(std::make_shared<connection::YamuxedConnection>(
        std::move(conn), scheduler_, close_cb_, config_));
  }
}  // namespace libp2p::muxer
