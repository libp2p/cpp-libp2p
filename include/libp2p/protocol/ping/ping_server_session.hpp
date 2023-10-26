/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/ping/ping_config.hpp>

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
