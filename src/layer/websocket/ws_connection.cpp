/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_connection.hpp>

#include <boost/asio/use_awaitable.hpp>
#include <libp2p/basic/read_return_size.hpp>
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
      setTimerPing();
    }
  }

  void WsConnection::stop() {
    if (not started_) {
      log_->error("already stopped (double stop)");
      return;
    }
    started_ = false;
    ping_handle_.reset();
    ping_timeout_handle_.reset();
  }

  void WsConnection::onPong(BytesIn payload) {
    auto expected = BytesIn(
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<const uint8_t *>(&ping_counter_),
        sizeof(ping_counter_));
    if (std::equal(
            payload.begin(), payload.end(), expected.begin(), expected.end())) {
      SL_DEBUG(log_, "Correct pong has received for ping");
      ping_timeout_handle_.reset();
      setTimerPing();
      return;
    }
    SL_DEBUG(log_, "Received unexpected pong. Ignoring");
  }

  bool WsConnection::isInitiator() const {
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

  void WsConnection::read(BytesOut out,
                          size_t bytes,
                          libp2p::basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    SL_TRACE(log_, "read {} bytes", bytes);
    readReturnSize(shared_from_this(), out, std::move(cb));
  }

  void WsConnection::readSome(BytesOut out,
                              size_t bytes,
                              libp2p::basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    SL_TRACE(log_, "read some upto {} bytes", bytes);
    auto on_read = [weak{weak_from_this()}, out, cb{std::move(cb)}](
                       boost::system::error_code ec, size_t n) mutable {
      if (ec) {
        cb(ec);
      } else if (n != 0) {
        cb(n);
      } else if (auto self = weak.lock()) {
        self->readSome(out, out.size(), std::move(cb));
      } else {
        cb(boost::system::errc::broken_pipe);
      }
    };
    ws_.async_read_some(asioBuffer(out), std::move(on_read));
  }

  void WsConnection::writeSome(BytesIn in,    //
                               size_t bytes,  //
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

  boost::asio::awaitable<outcome::result<size_t>> WsConnection::read(
      BytesOut out, size_t bytes) {
    ambigousSize(out, bytes);
    SL_TRACE(log_, "read {} bytes (coroutine)", bytes);

    if (bytes == 0 || out.empty()) {
      co_return outcome::success(0);
    }

    size_t total_bytes_read = 0;

    while (total_bytes_read < bytes) {
      auto result = co_await readSome(BytesOut{out.data() + total_bytes_read,
                                               out.size() - total_bytes_read},
                                      bytes - total_bytes_read);

      if (!result) {
        co_return result.error();
      }

      size_t bytes_read = result.value();
      if (bytes_read == 0) {
        break;  // EOF reached
      }

      total_bytes_read += bytes_read;
    }

    co_return outcome::success(total_bytes_read);
  }

  boost::asio::awaitable<outcome::result<size_t>> WsConnection::readSome(
      BytesOut out, size_t bytes) {
    ambigousSize(out, bytes);
    SL_TRACE(log_, "read some upto {} bytes (coroutine)", bytes);

    if (bytes == 0 || out.empty()) {
      co_return outcome::success(0);
    }

    try {
      size_t n = co_await ws_.async_read_some(
          asioBuffer(out), boost::asio::use_awaitable);

      if (n != 0) {
        co_return outcome::success(n);
      }

      // If we got zero bytes and we're still connected, try again
      if (!isClosed()) {
        auto result = co_await readSome(out, out.size());
        co_return result;
      }

      co_return outcome::failure(boost::system::errc::broken_pipe);
    } catch (const boost::system::system_error &error) {
      co_return outcome::failure(error.code());
    } catch (const std::exception &) {
      co_return outcome::failure(boost::system::errc::io_error);
    }
  }

  boost::asio::awaitable<std::error_code> WsConnection::writeSome(
      BytesIn in, size_t bytes) {
    ambigousSize(in, bytes);
    SL_TRACE(log_, "write some upto {} bytes (coroutine)", bytes);

    if (bytes == 0 || in.empty()) {
      co_return std::error_code{};
    }

    try {
      co_await ws_.async_write_some(
          true, asioBuffer(in), boost::asio::use_awaitable);
      co_return std::error_code{};
    } catch (const boost::system::system_error &error) {
      co_return error.code();
    } catch (const std::exception &) {
      co_return boost::system::errc::make_error_code(
          boost::system::errc::io_error);
    }
  }

  void WsConnection::setTimerPing() {
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
}  // namespace libp2p::connection
