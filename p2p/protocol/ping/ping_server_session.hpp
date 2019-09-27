/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PING_SERVER_SESSION_HPP
#define LIBP2P_PING_SERVER_SESSION_HPP

#include <memory>
#include <vector>

#include "connection/stream.hpp"
#include "host/host.hpp"
#include "protocol/ping/ping_config.hpp"

namespace libp2p::protocol {
  class PingServerSession
      : public std::enable_shared_from_this<PingServerSession> {
   public:
    PingServerSession(std::shared_ptr<connection::Stream> stream,
                      PingConfig config);

    void start();

   private:
    void read();

    void readCompleted();

    void write();

    void writeCompleted();

    std::shared_ptr<connection::Stream> stream_;
    PingConfig config_;

    std::vector<uint8_t> buffer_;

    bool is_started_ = false;
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_PING_SERVER_SESSION_HPP
