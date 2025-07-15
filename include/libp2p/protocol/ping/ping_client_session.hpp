/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <vector>

#include <libp2p/basic/scheduler.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/crypto/random_generator.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/protocol/ping/ping_config.hpp>

namespace libp2p::peer {
  class PeerId;
}

namespace libp2p::event::protocol {

  /// emitted when Ping timeout is expired, or error happens during the ping
  /// process
  using PeerIsDeadChannel =
      channel_decl<struct PeerIsDead, libp2p::peer::PeerId>;

}  // namespace libp2p::event::protocol

namespace libp2p::protocol {

  class PingClientSession
      : public std::enable_shared_from_this<PingClientSession> {
   public:
    PingClientSession(std::shared_ptr<basic::Scheduler> scheduler,
                      libp2p::event::Bus &bus,
                      std::shared_ptr<connection::Stream> stream,
                      std::shared_ptr<crypto::random::RandomGenerator> rand_gen,
                      PingConfig config);

    void start();

    void stop();

   private:
    void write();

    void writeCompleted(outcome::result<void> r);

    void read();

    void readCompleted(outcome::result<void> r);

    void close();

    std::shared_ptr<basic::Scheduler> scheduler_;
    libp2p::event::Bus &bus_;
    decltype(bus_.getChannel<event::protocol::PeerIsDeadChannel>()) channel_;

    std::shared_ptr<connection::Stream> stream_;
    std::shared_ptr<crypto::random::RandomGenerator> rand_gen_;
    PingConfig config_;

    std::vector<uint8_t> write_buffer_, read_buffer_;
    basic::Scheduler::Handle timer_;
    bool closed_ = false;

    bool is_started_ = false;
  };
}  // namespace libp2p::protocol
