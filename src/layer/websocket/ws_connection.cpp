/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_connection.hpp>

#include <boost/asio/read.hpp>
#include <libp2p/common/ambigous_size.hpp>
#include <libp2p/common/asio_buffer.hpp>
#include <libp2p/common/asio_cb.hpp>
#include <libp2p/common/bytestr.hpp>
#include <libp2p/log/logger.hpp>

namespace libp2p::connection {
  WsConnection::WsConnection(
      layer::WsConnectionConfig config,
      std::shared_ptr<boost::asio::io_context> io_context,
      std::shared_ptr<LayerConnection> connection,
      std::shared_ptr<basic::Scheduler> scheduler)
      : config_(std::move(config)),
        connection_(std::move(connection)),
        ws_(AsAsioReadWrite{std::move(io_context), connection_}),
        scheduler_(std::move(scheduler)) {
    BOOST_ASSERT(connection_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    ws_.binary(true);
  }

  void WsConnection::start() {
    if (started_) {
      log_->error("already started (double start)");
      return;
    }
    started_ = true;

    if (config_.ping_interval != std::chrono::milliseconds::zero()) {
      // Set pong handler
      using boost::beast::websocket::frame_type;
      ws_.control_callback(
          [weak = weak_from_this()](frame_type type,
                                    boost::beast::string_view payload) {
            if (type != frame_type::pong) {
              return;
            }
            if (auto self = weak.lock()) {
              self->onPong(bytestr(payload));
            }
          });

      ping_handle_ = scheduler_->scheduleWithHandle(
          [wp = weak_from_this()]() mutable {
            if (auto self = wp.lock(); self and self->started_) {
              ++self->ping_counter_;

              // Send ping
              self->ws_.async_ping(
                  {
                      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                      reinterpret_cast<const char *>(&self->ping_counter_),
                      sizeof(self->ping_counter_),
                  },
                  [](boost::system::error_code) {});

              // Start timer of pong waiting
              self->ping_timeout_handle_ = self->scheduler_->scheduleWithHandle(
                  [wp] {
                    if (auto self = wp.lock()) {
                      self->ws_.async_close(
                          {
                              boost::beast::websocket::close_code::policy_error,
                              "Pong was not received on time",
                          },
                          [](boost::system::error_code) {});
                    }
                  },
                  self->config_.ping_timeout);
            }
          },
          config_.ping_interval);
    }
  }

  void WsConnection::stop() {
    if (not started_) {
      log_->error("already stopped (double stop)");
      return;
    }
    started_ = false;
    ping_handle_.cancel();
    ping_timeout_handle_.cancel();
  }

  void WsConnection::onPong(gsl::span<const uint8_t> payload) {
    auto expected = gsl::make_span(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t *>(&ping_counter_),
        sizeof(ping_counter_));
    if (payload == expected) {
      SL_DEBUG(log_, "Correct pong has received for ping");
      ping_timeout_handle_.cancel();
      std::ignore = ping_handle_.reschedule(config_.ping_interval);
      return;
    }
    SL_DEBUG(log_, "Received unexpected pong. Ignoring");
  }

  bool WsConnection::isInitiator() const noexcept {
    return connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> WsConnection::localMultiaddr() {
    return connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> WsConnection::remoteMultiaddr() {
    return connection_->remoteMultiaddr();
  }

  bool WsConnection::isClosed() const {
    return !started_ || connection_->isClosed();
  }

  outcome::result<void> WsConnection::close() {
    return connection_->close();
  }

  void WsConnection::read(gsl::span<uint8_t> out, size_t bytes,
                          libp2p::basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    SL_TRACE(log_, "read {} bytes", bytes);
    boost::asio::async_read(ws_, asioBuffer(out), toAsioCbSize(std::move(cb)));
  }

  void WsConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                              libp2p::basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    SL_TRACE(log_, "read some upto {} bytes", bytes);
    ws_.async_read_some(asioBuffer(out), toAsioCbSize(std::move(cb)));
  }

  void WsConnection::write(gsl::span<const uint8_t> in,  //
                           size_t bytes,                 //
                           libp2p::basic::Writer::WriteCallbackFunc cb) {
    ambigousSize(in, bytes);
    SL_TRACE(log_, "write {} bytes", bytes);
    ws_.async_write(asioBuffer(in), toAsioCbSize(std::move(cb)));
  }

  void WsConnection::writeSome(gsl::span<const uint8_t> in,  //
                               size_t bytes,                 //
                               libp2p::basic::Writer::WriteCallbackFunc cb) {
    ambigousSize(in, bytes);
    SL_TRACE(log_, "write some upto {} bytes", bytes);
    ws_.async_write_some(true, asioBuffer(in), toAsioCbSize(std::move(cb)));
  }

  void WsConnection::deferReadCallback(outcome::result<size_t> res,
                                       ReadCallbackFunc cb) {
    connection_->deferReadCallback(res, std::move(cb));
  }

  void WsConnection::deferWriteCallback(std::error_code ec,
                                        WriteCallbackFunc cb) {
    connection_->deferWriteCallback(ec, std::move(cb));
  }
}  // namespace libp2p::connection
