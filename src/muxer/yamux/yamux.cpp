/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux.hpp>

#include <libp2p/muxer/yamux/yamuxed_connection.hpp>

namespace libp2p::muxer {
  Yamux::Yamux(MuxedConnectionConfig config,
               std::shared_ptr<basic::Scheduler> scheduler)
      : config_{config}, scheduler_{std::move(scheduler)} {
    assert(scheduler_);
  }

  peer::Protocol Yamux::getProtocolId() const noexcept {
    return "/yamux/1.0.0";
  }

  void Yamux::muxConnection(std::shared_ptr<connection::SecureConnection> conn,
                            CapConnCallbackFunc cb) const {
    cb(std::make_shared<connection::YamuxedConnection>(std::move(conn),
                                                       scheduler_,
                                                       config_));
  }
}  // namespace libp2p::muxer
