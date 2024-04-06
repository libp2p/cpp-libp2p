/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol/ping/ping_client_session.hpp>

#include <boost/assert.hpp>
#include <libp2p/basic/write_return_size.hpp>
#include <libp2p/protocol/ping/common.hpp>

namespace libp2p::protocol {
  PingClientSession::PingClientSession(
      std::shared_ptr<basic::Scheduler> scheduler,
      libp2p::event::Bus &bus,
      std::shared_ptr<connection::Stream> stream,
      std::shared_ptr<crypto::random::RandomGenerator> rand_gen,
      PingConfig config)
      : scheduler_{scheduler},
        bus_{bus},
        channel_{bus_.getChannel<event::protocol::PeerIsDeadChannel>()},
        stream_{std::move(stream)},
        rand_gen_{std::move(rand_gen)},
        config_{config},
        write_buffer_(config_.message_size, 0),
        read_buffer_(config_.message_size, 0) {
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
    if (not is_started_ or closed_ or stream_->isClosedForWrite()) {
      return;
    }

    timer_ = scheduler_->scheduleWithHandle(
        [weak{weak_from_this()}] {
          if (auto self = weak.lock()) {
            self->writeCompleted(std::errc::timed_out);
          }
        },
        config_.timeout);

    write_buffer_ = rand_gen_->randomBytes(config_.message_size);
    writeReturnSize(stream_,
                    write_buffer_,
                    [self{shared_from_this()}](outcome::result<size_t> r) {
                      self->writeCompleted(r);
                    });
  }

  void PingClientSession::writeCompleted(outcome::result<size_t> r) {
    timer_.cancel();
    if (r.has_error()) {
      // timeout passed or error happened; in any case, we cannot ping it
      // anymore
      return close();
    }
    read();
  }

  void PingClientSession::read() {
    if (not is_started_ or closed_ or stream_->isClosedForRead()) {
      return;
    }

    timer_ = scheduler_->scheduleWithHandle(
        [weak{weak_from_this()}] {
          if (auto self = weak.lock()) {
            self->readCompleted(std::errc::timed_out);
          }
        },
        config_.timeout);

    stream_->read(read_buffer_,
                  config_.message_size,
                  [self{shared_from_this()}](outcome::result<size_t> r) {
                    self->readCompleted(r);
                  });
  }

  void PingClientSession::readCompleted(outcome::result<size_t> r) {
    timer_.cancel();
    if (r.has_error() || write_buffer_ != read_buffer_) {
      // again, in case of any error we cannot continue to ping the peer and
      // thus declare it dead
      return close();
    }

    timer_ = scheduler_->scheduleWithHandle(
        [weak{weak_from_this()}] {
          if (auto self = weak.lock()) {
            self->write();
          }
        },
        config_.interval);
  }

  void PingClientSession::close() {
    if (closed_) {
      return;
    }
    closed_ = true;
    if (auto peer_id_res = stream_->remotePeerId()) {
      channel_.publish(peer_id_res.value());
    }
    stream_->reset();
  }
}  // namespace libp2p::protocol
