/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/mplex/mplex.hpp>

#include <memory>

#include <libp2p/muxer/mplex/mplexed_connection.hpp>

namespace libp2p::muxer {
  Mplex::Mplex(MuxedConnectionConfig config) : config_{config} {}

  peer::ProtocolName Mplex::getProtocolId() const noexcept {
    return "/mplex/6.7.0";
  }

  void Mplex::muxConnection(std::shared_ptr<connection::SecureConnection> conn,
                            CapConnCallbackFunc cb) const {
    cb(std::make_shared<connection::MplexedConnection>(std::move(conn),
                                                       config_));
  }
}  // namespace libp2p::muxer
