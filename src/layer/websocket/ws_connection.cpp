/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_connection.hpp>

#include <libp2p/log/logger.hpp>

namespace libp2p::connection {

  WsConnection::WsConnection(
      std::shared_ptr<const layer::WsConnectionConfig> config,
      std::shared_ptr<LayerConnection> connection,
      std::shared_ptr<basic::Scheduler> scheduler,
      gsl::span<const uint8_t> preloaded_data)
      : config_([&] {
          return std::dynamic_pointer_cast<const layer::WsConnectionConfig>(
              config);
        }()),
        connection_(std::move(connection)),
        scheduler_(std::move(scheduler)),
        ws_read_writer_(std::make_shared<websocket::WsReadWriter>(
            scheduler_, connection_, preloaded_data)) {
    BOOST_ASSERT(config_ != nullptr);
    BOOST_ASSERT(connection_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
  }

  void WsConnection::start() {
    if (started_) {
      log_->error("already started (double start)");
      return;
    }
    started_ = true;

    if (config_->ping_interval != std::chrono::milliseconds::zero()) {
      // Set pong handler
      ws_read_writer_->setPongHandler([wp = weak_from_this()](auto payload) {
        if (auto self = wp.lock()) {
          self->onPong(payload);
        }
      });

      ping_handle_ = scheduler_->scheduleWithHandle(
          [wp = weak_from_this()]() mutable {
            if (auto self = wp.lock(); self and self->started_) {
              ++self->ping_counter_;

              auto payload = gsl::make_span(
                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                  reinterpret_cast<const uint8_t *>(&self->ping_counter_),
                  sizeof(self->ping_counter_));
              // Send ping
              self->ws_read_writer_->ping(payload);

              // Start timer of pong waiting
              self->ping_timeout_handle_ =
                  self->scheduler_->scheduleWithHandle([wp] {
                    if (auto self = wp.lock()) {
                      self->ws_read_writer_->close(
                          websocket::WsReadWriter::ReasonOfClose::
                              PINGING_TIMEOUT);
                    }
                  });
            }
          },
          config_->ping_interval);
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
      std::ignore = ping_handle_.reschedule(config_->ping_interval);
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
    OperationContext context{.bytes_served = 0,
                             .total_bytes = bytes,
                             .write_buffer_it = write_buffers_.end()};
    SL_TRACE(log_, "read {} bytes", bytes);
    read(out, bytes, context, std::move(cb));
  }

  void WsConnection::read(gsl::span<uint8_t> out, size_t bytes,
                          OperationContext ctx, ReadCallbackFunc cb) {
    SL_TRACE(log_, "read {} bytes (ctx: served={} total={})", bytes,
             ctx.bytes_served, ctx.total_bytes);

    size_t out_size = out.empty() ? 0u : static_cast<size_t>(out.size());
    BOOST_ASSERT(out_size >= bytes);
    if (bytes == 0) {
      BOOST_ASSERT(ctx.bytes_served == ctx.total_bytes);
      return cb(ctx.bytes_served);
    }
    readSome(out, bytes,
             [self{shared_from_this()},  //
              out,                       //
              bytes,                     //
              cb{std::move(cb)},         //
              ctx]                       //
             (auto res) mutable {
               if (res.has_error()) {
                 SL_DEBUG(self->log_, "Can't read data: {}",
                          res.error().message());
                 return cb(res.as_failure());
               }
               auto read_bytes = res.value();
               ctx.bytes_served += read_bytes;
               self->read(out.subspan(read_bytes), bytes - read_bytes, ctx,
                          std::move(cb));
             });
  }

  void WsConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                              libp2p::basic::Reader::ReadCallbackFunc cb) {
    OperationContext context{.bytes_served = 0,
                             .total_bytes = bytes,
                             .write_buffer_it = write_buffers_.end()};
    SL_TRACE(log_, "read some upto {} bytes", bytes);
    readSome(out, bytes, context, std::move(cb));
  }

  void WsConnection::readSome(gsl::span<uint8_t> out, size_t required_bytes,
                              OperationContext ctx, ReadCallbackFunc cb) {
    SL_TRACE(log_, "read some upto {} bytes (ctx: served={} total={})",
             required_bytes, ctx.bytes_served, ctx.total_bytes);

    // Return available data if any
    if (not read_buffers_.empty()) {
      size_t copied_bytes = 0;

      while (not read_buffers_.empty()) {
        auto &buffer = read_buffers_.front();

        auto available_bytes =
            std::min<size_t>(required_bytes - copied_bytes, buffer.size());

        SL_TRACE(log_, "inner buffer: {} bytes", buffer.size());

        auto in_begin = buffer.begin();
        auto in_end =
            std::next(in_begin, static_cast<ssize_t>(available_bytes));
        auto out_begin =
            std::next(out.begin(), static_cast<ssize_t>(copied_bytes));

        SL_TRACE(log_, "copy {} first bytes", available_bytes);

        std::copy(in_begin, in_end, out_begin);
        copied_bytes += available_bytes;

        if (available_bytes < buffer.size()) {
          buffer.erase(in_begin, in_end);
          SL_TRACE(log_, "inner buffer truncated to {} bytes", buffer.size());
          break;
        }

        read_buffers_.pop_front();
        SL_TRACE(log_, "inner buffer has dropped", buffer.size());
      }

      if (copied_bytes > 0) {
        return cb(copied_bytes);
      }
    }

    // Otherwise try to read
    ws_read_writer_->read([self = shared_from_this(),  //
                           out,                        //
                           required_bytes,             //
                           cb = std::move(cb),         //
                           ctx]                        //
                          (auto res) mutable {
                            if (res.has_error()) {
                              SL_DEBUG(self->log_, "Can't read some data: {}",
                                       res.error().message());
                              return cb(res.as_failure());
                            }
                            auto &data = res.value();

                            self->read_buffers_.emplace_back(std::move(*data));
                            self->readSome(out, required_bytes, ctx,
                                           std::move(cb));
                          });
  }

  void WsConnection::write(gsl::span<const uint8_t> in,  //
                           size_t bytes,                 //
                           libp2p::basic::Writer::WriteCallbackFunc cb) {
    SL_TRACE(log_, "write {} bytes", bytes);

    ws_read_writer_->write(
        in,
        [self = shared_from_this(),                         //
         in = in.subspan(static_cast<int64_t>(in.size())),  //
         bytes = bytes - in.size(),                         //
         cb = std::move(cb)]                                //
        (auto res) mutable {
          if (res.has_error()) {
            SL_DEBUG(self->log_, "Can't write data: {}", res.error().message());
            return cb(res.as_failure());
          }
          auto &writen_bytes = res.value();
          return cb(writen_bytes);
        });
  }

  void WsConnection::writeSome(gsl::span<const uint8_t> in,  //
                               size_t bytes,                 //
                               libp2p::basic::Writer::WriteCallbackFunc cb) {
    SL_TRACE(log_, "write some upto {} bytes", bytes);
    write(in, bytes, std::move(cb));
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
