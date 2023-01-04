/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_connection.hpp>

#include <boost/asio/error.hpp>

//#include <libp2p/layer/websocket/ws_frame.hpp>
#include <libp2p/log/logger.hpp>

#define PP_CAT(a, b) PP_CAT_I(a, b)
#define PP_CAT_I(a, b) PP_CAT_II(~, a##b)
#define PP_CAT_II(p, res) res

#define UNIQUE_NAME(base) PP_CAT(base, __LINE__)

#define OUTCOME_CB_I(var, res)                   \
  auto && (var) = (res);                         \
  if ((var).has_error()) {                       \
    self->eraseWriteBuffer(ctx.write_buffer_it); \
    return cb((var).as_failure());               \
  }

#define OUTCOME_CB_NAME_I(var, val, res) \
  OUTCOME_CB_I(var, res)                 \
  [[maybe_unused]] auto && (val) = (var).value();

#define OUTCOME_CB(name, res) OUTCOME_CB_NAME_I(UNIQUE_NAME(name), name, res)

namespace libp2p::connection {

  WsConnection::WsConnection(
      std::shared_ptr<const layer::WsConnectionConfig> config,
      std::shared_ptr<LayerConnection> connection,
      std::shared_ptr<basic::Scheduler> scheduler)
      : config_([&] {
          return std::dynamic_pointer_cast<const layer::WsConnectionConfig>(
              config);
        }()),
        connection_(std::move(connection)),
        scheduler_(std::move(scheduler)),
        raw_read_buffer_(std::make_shared<Buffer>()),
        ws_read_writer_(std::make_shared<websocket::WsReadWriter>(
            connection_, raw_read_buffer_)) {
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
      ping_handle_ = scheduler_->scheduleWithHandle(
          [this, ping_counter = 0u]() mutable {
            if (started_) {
              // don't send pings if something is being written
              if (!is_writing_) {
                ++ping_counter;

                ws_read_writer_->sendPing(
                    gsl::make_span(reinterpret_cast<const uint8_t *>(  // NOLINT
                                       &ping_counter),
                                   sizeof(ping_counter)),
                    [](const auto &res) {
                      // TODO handle sent ping
                    });

                SL_TRACE(log_, "written ping message #{}", ping_counter);
              }
              std::ignore = ping_handle_.reschedule(config_->ping_interval);
            }
          },
          config_->ping_interval);
    }
  }

  void WsConnection::stop() {
    if (!started_) {
      log_->error("already stopped (double stop)");
      return;
    }
    ping_handle_.cancel();
    started_ = false;
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
    read(out, bytes, context, std::move(cb));
  }

  void WsConnection::read(gsl::span<uint8_t> out, size_t bytes,
                          OperationContext ctx, ReadCallbackFunc cb) {
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
               OUTCOME_CB(read_bytes, res);
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
    readSome(out, bytes, context, std::move(cb));
  }

  void WsConnection::readSome(gsl::span<uint8_t> out, size_t required_bytes,
                              OperationContext ctx, ReadCallbackFunc cb) {
    // Return available data if any
    if (not frame_buffer_->empty()) {
      auto available_bytes =
          std::min<ssize_t>(required_bytes, frame_buffer_->size());
      auto begin = frame_buffer_->begin();
      auto end = std::next(begin, available_bytes);

      std::copy(begin, end, out.begin());
      frame_buffer_->erase(begin, end);

      return cb(available_bytes);
    }

    // Otherwise try to read
    ws_read_writer_->read([self = shared_from_this(),  //
                           out,                        //
                           required_bytes,             //
                           cb = std::move(cb),         //
                           ctx]                        //
                          (auto res) mutable {
                            OUTCOME_CB(data, res);
                            self->frame_buffer_ = std::move(res.value());
                            self->readSome(out, required_bytes, ctx,
                                           std::move(cb));
                          });
  }

  void WsConnection::write(gsl::span<const uint8_t> in,  //
                           size_t bytes,                 //
                           libp2p::basic::Writer::WriteCallbackFunc cb) {
    OperationContext context{.bytes_served = 0,
                             .total_bytes = bytes,
                             .write_buffer_it = write_buffers_.end()};
    write(in, bytes, context, std::move(cb));
  }

  void WsConnection::write(gsl::span<const uint8_t> in,  //
                           size_t bytes,                 //
                           WsConnection::OperationContext ctx,
                           basic::Writer::WriteCallbackFunc cb) {
    if (0 == bytes) {
      BOOST_ASSERT(ctx.bytes_served >= ctx.total_bytes);
      eraseWriteBuffer(ctx.write_buffer_it);
      return cb(ctx.total_bytes);
    }

    if (ctx.write_buffer_it == write_buffers_.end()) {
      ctx.write_buffer_it = write_buffers_.emplace(write_buffers_.end());
    }

    ctx.write_buffer_it->assign(in.begin(), in.end());

    ws_read_writer_->write(
        *ctx.write_buffer_it,
        [self = shared_from_this(),                         //
         in = in.subspan(static_cast<int64_t>(in.size())),  //
         bytes = bytes - in.size(),                         //
         cb = std::move(cb),                                //
         ctx]                                               //
        (auto res) mutable {
          OUTCOME_CB(writen_bytes, res);
          ctx.bytes_served += writen_bytes;
          self->write(in, bytes, ctx, std::move(cb));
        });
  }

  void WsConnection::writeSome(gsl::span<const uint8_t> in,  //
                               size_t bytes,                 //
                               libp2p::basic::Writer::WriteCallbackFunc cb) {
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

  void WsConnection::onExpireTimer() {
    // if (streams_.empty() && pending_outbound_streams_.empty()) {
    //   SL_DEBUG(log_, "closing expired connection");
    //   close(Error::CONNECTION_NOT_ACTIVE, WsFrame::GoAwayError::NORMAL);
    // }
  }

  void WsConnection::eraseWriteBuffer(BufferList::iterator &iterator) {
    if (write_buffers_.end() == iterator) {
      return;
    }
    write_buffers_.erase(iterator);
    iterator = write_buffers_.end();
  }

}  // namespace libp2p::connection
