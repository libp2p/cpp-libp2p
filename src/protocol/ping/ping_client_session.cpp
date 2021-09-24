/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/ping/ping_client_session.hpp>

#include <boost/asio/deadline_timer.hpp>
#include <boost/assert.hpp>
#include <libp2p/protocol/ping/common.hpp>

namespace libp2p::protocol {
  PingClientSession::PingClientSession(
      boost::asio::io_service &io_service, libp2p::event::Bus &bus,
      std::shared_ptr<connection::Stream> stream,
      std::shared_ptr<crypto::random::RandomGenerator> rand_gen,
      PingConfig config)
      : io_service_{io_service},
        bus_{bus},
        channel_{bus_.getChannel<event::protocol::PeerIsDeadChannel>()},
        stream_{std::move(stream)},
        rand_gen_{std::move(rand_gen)},
        config_{config},
        write_buffer_(config_.message_size, 0),
        read_buffer_(config_.message_size, 0),
        timer_{io_service_, boost::posix_time::milliseconds(config_.timeout)} {
    BOOST_ASSERT(stream_);
    BOOST_ASSERT(rand_gen_);
  }

  void PingClientSession::start() {
    BOOST_ASSERT(!is_started_);
    is_started_ = true;
    write();
  }

  void PingClientSession::stop() {
    BOOST_ASSERT(is_started_);
    is_started_ = false;
  }

  void PingClientSession::write() {
    if (!is_started_ || stream_->isClosedForWrite()) {
      return;
    }

    auto rand_buf = rand_gen_->randomBytes(config_.message_size);
    std::move(rand_buf.begin(), rand_buf.end(), write_buffer_.begin());
    stream_->write(write_buffer_, config_.message_size,
                   [self{shared_from_this()}](auto &&write_res) {
                     if (!write_res) {
                       self->last_error_ = write_res.error();
                     }
                     self->last_op_completed_ = true;
                   });

    timer_.async_wait([self{shared_from_this()}](auto &&ec) {
      self->writeCompleted(std::forward<decltype(ec)>(ec));
    });
  }

  void PingClientSession::writeCompleted(const boost::system::error_code &ec) {
    if (ec || !last_op_completed_ || last_error_) {
      // timeout passed or error happened; in any case, we cannot ping it
      // anymore
      if (auto peer_id_res = stream_->remotePeerId()) {
        channel_.publish(peer_id_res.value());
      }
      return;
    }
    last_op_completed_ = false;
    read();
  }

  void PingClientSession::read() {
    if (!is_started_ || stream_->isClosedForRead()) {
      return;
    }

    stream_->read(read_buffer_, config_.message_size,
                  [self{shared_from_this()}](auto &&read_res) {
                    if (!read_res) {
                      self->last_error_ = read_res.error();
                    }
                    self->last_op_completed_ = true;
                  });

    timer_.async_wait([self{shared_from_this()}](auto &&ec) {
      self->readCompleted(std::forward<decltype(ec)>(ec));
    });
  }

  void PingClientSession::readCompleted(const boost::system::error_code &ec) {
    if (ec || !last_op_completed_ || last_error_
        || write_buffer_ != read_buffer_) {
      // again, in case of any error we cannot continue to ping the peer and
      // thus declare it dead
      if (auto peer_id_res = stream_->remotePeerId()) {
        channel_.publish(peer_id_res.value());
      }
      return;
    }
    last_op_completed_ = false;
    write();
  }
}  // namespace libp2p::protocol
