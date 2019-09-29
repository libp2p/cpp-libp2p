/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PING_CLIENT_SESSION_HPP
#define LIBP2P_PING_CLIENT_SESSION_HPP

#include <memory>
#include <vector>

#include <boost/asio/io_service.hpp>
#include <libp2p/connection/stream.hpp>
#include <libp2p/crypto/random_generator.hpp>
#include <libp2p/event/bus.hpp>
#include <libp2p/protocol/ping/ping_config.hpp>

namespace libp2p::peer {
  class PeerId;
}

namespace libp2p::protocol {
  namespace event {
    /// emitted when Ping timeout is expired, or error happens during the ping
    /// process
    struct PeerIsDead {};
    using PeerIsDeadChannel =
        libp2p::event::channel_decl<PeerIsDead, peer::PeerId>;
  }  // namespace event

  class PingClientSession
      : public std::enable_shared_from_this<PingClientSession> {
   public:
    PingClientSession(boost::asio::io_service &io_service,
                      libp2p::event::Bus &bus,
                      std::shared_ptr<connection::Stream> stream,
                      std::shared_ptr<crypto::random::RandomGenerator> rand_gen,
                      PingConfig config);

    void start();

    void stop();

   private:
    void write();

    void writeCompleted(const boost::system::error_code &ec);

    void read();

    void readCompleted(const boost::system::error_code &ec);

    boost::asio::io_service &io_service_;
    libp2p::event::Bus &bus_;
    decltype(bus_.getChannel<event::PeerIsDeadChannel>()) channel_;

    std::shared_ptr<connection::Stream> stream_;
    std::shared_ptr<crypto::random::RandomGenerator> rand_gen_;
    PingConfig config_;

    std::vector<uint8_t> write_buffer_, read_buffer_;
    boost::asio::deadline_timer timer_;

    bool last_op_completed_ = false;
    std::error_code last_error_;

    bool is_started_ = false;
  };
}  // namespace libp2p::protocol

#endif  // LIBP2P_PING_CLIENT_SESSION_HPP
