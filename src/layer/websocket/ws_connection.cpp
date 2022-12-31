/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/layer/websocket/ws_connection.hpp>

#include <boost/asio/error.hpp>

#include <libp2p/layer/websocket/ws_frame.hpp>
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
        reading_state_(
            [this](boost::optional<websocket::WsFrame> header) {
              return processHeader(std::move(header));
            },
            [this](gsl::span<uint8_t> segment, bool fin) {
              if (!segment.empty()) {
                processData(segment);
              }
              if (fin) {
                processFin();
              }
            }) {
    BOOST_ASSERT(config_ != nullptr);
    BOOST_ASSERT(connection_ != nullptr);
    BOOST_ASSERT(scheduler_ != nullptr);
    raw_read_buffer_->resize(websocket::WsFrame::kInitialWindowSize + 4096);
    framer_ = std::make_shared<websocket::WsReadWriter>(connection_,
                                                        raw_read_buffer_);
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
                enqueue(websocket::pingOutMsg(++ping_counter));
                SL_TRACE(log_, "written ping message #{}", ping_counter);
              }
              std::ignore = ping_handle_.reschedule(config_->ping_interval);
            }
          },
          config_->ping_interval);
    }

    continueReading();
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
    size_t out_size{out.empty() ? 0u : static_cast<size_t>(out.size())};
    BOOST_ASSERT(out_size >= bytes);
    if (0 == bytes) {
      BOOST_ASSERT(ctx.bytes_served == ctx.total_bytes);
      return cb(ctx.bytes_served);
    }
    readSome(out, bytes,
             // clang-format off
             [
               self{shared_from_this()},
               out,
               bytes,
               cb{std::move(cb)},
               ctx
             ]
             // clang-format on
             (auto _n) mutable {
               OUTCOME_CB(n, _n);
               ctx.bytes_served += n;
               self->read(out.subspan(n), bytes - n, ctx, std::move(cb));
             });
  }

  void WsConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                              libp2p::basic::Reader::ReadCallbackFunc cb) {
    OperationContext context{.bytes_served = 0,
                             .total_bytes = bytes,
                             .write_buffer_it = write_buffers_.end()};
    readSome(out, bytes, context, std::move(cb));
  }

  void WsConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                              OperationContext ctx, ReadCallbackFunc cb) {
    if (not frame_buffer_->empty()) {
      auto n{std::min(bytes, frame_buffer_->size())};
      auto begin{frame_buffer_->begin()};
      auto end{begin + static_cast<int64_t>(n)};
      std::copy(begin, end, out.begin());
      frame_buffer_->erase(begin, end);
      return cb(n);
    }
    framer_->read([self{shared_from_this()}, out, bytes, cb{std::move(cb)},
                   ctx](auto _data) mutable {
      OUTCOME_CB(data, _data);
      //  OUTCOME_CB(decrypted, self->decoder_cs_->decrypt({}, *data, {}));
      //  self->frame_buffer_->assign(decrypted.begin(), decrypted.end());
      self->readSome(out, bytes, ctx, std::move(cb));
    });
  }

  void WsConnection::write(gsl::span<const uint8_t> in, size_t bytes,
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

    if (write_buffers_.end() == ctx.write_buffer_it) {
      ctx.write_buffer_it = write_buffers_.emplace(write_buffers_.end());
    }

    auto n = std::min(bytes, websocket::WsFrame::kMaxDataSize);
    auto x = in.subspan(0, n);

    ctx.write_buffer_it->assign(x.begin(), x.end());

    framer_->write(
        *ctx.write_buffer_it,
        // clang-format off
        [
            self{shared_from_this()},
            in{in.subspan(static_cast<int64_t>(n))},
            bytes{bytes - n},
            cb{std::move(cb)},
            ctx
        ]
        // clang-format on
        (auto _n) mutable {
          OUTCOME_CB(n, _n);
          ctx.bytes_served += n;
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

  void WsConnection::continueReading() {
    SL_TRACE(log_, "continueReading");
    connection_->readSome(*raw_read_buffer_, raw_read_buffer_->size(),
                          [wptr = weak_from_this(), buffer = raw_read_buffer_](
                              outcome::result<size_t> res) {
                            auto self = wptr.lock();
                            if (self) {
                              self->onRead(res);
                            }
                          });
  }

  void WsConnection::onRead(outcome::result<size_t> res) {
    if (!started_) {
      return;
    }

    if (!res) {
      std::error_code ec = res.error();
      if (ec.value() == boost::asio::error::eof) {
        ec = Error::CONNECTION_CLOSED_BY_PEER;
      }
      close(ec, boost::none);
      return;
    }

    auto n = res.value();
    gsl::span<uint8_t> bytes_read(*raw_read_buffer_);

    SL_TRACE(log_, "read {} bytes", n);

    assert(n <= raw_read_buffer_->size());

    if (n < raw_read_buffer_->size()) {
      bytes_read = bytes_read.first(ssize_t(n));
    }

    reading_state_.onDataReceived(bytes_read);

    if (!started_) {
      return;
    }

    continueReading();
  }

  bool WsConnection::processHeader(boost::optional<websocket::WsFrame> header) {
    using FrameType = websocket::WsFrame::FrameType;

    if (!header) {
      SL_DEBUG(log_, "cannot parse yamux frame: corrupted");
      close(Error::CONNECTION_PROTOCOL_ERROR,
            websocket::WsFrame::GoAwayError::PROTOCOL_ERROR);
      return false;
    }

    SL_TRACE(log_, "WsConnection::processHeader");

    auto &frame = header.value();

    if (frame.type == FrameType::GO_AWAY) {
      return false;
    }

    // bool is_fin = frame.flagIsSet(websocket::WsFrame::Flag::FIN);
    //
    // if (is_fin && (frame.stream_id != 0)) {
    //   processFin();
    // }

    // proceed with incoming data
    return true;
  }

  void WsConnection::processData(gsl::span<uint8_t> segment) {
    assert(!segment.empty());

    SL_TRACE(log_, "WsConnection::processData, size={}", segment.size());

    //  auto result = it->second->onDataReceived(segment);
    //  if (result == WsStream::kKeepStream) {
    //    return;
    //  }
    //
    //  reading_state_.discardDataMessage();
    //
    //  if (result == WsStream::kRemoveStreamAndSendRst) {
    //    // overflow, reset this stream
    //    enqueue(resetStreamMsg(stream_id));
    //  }
  }

  void WsConnection::processFin() {
    SL_DEBUG(log_, "received Fin frame");
    std::ignore = connection_->close();
  }

  void WsConnection::close(
      std::error_code notify_streams_code,
      boost::optional<websocket::WsFrame::GoAwayError> reply_to_peer_code) {
    if (!started_) {
      return;
    }

    started_ = false;

    SL_DEBUG(log_, "closing connection, reason: {}",
             notify_streams_code.message());

    write_queue_.clear();

    if (reply_to_peer_code.has_value() && !connection_->isClosed()) {
      enqueue(goAwayMsg(reply_to_peer_code.value()));
    }

    //    Streams streams;
    //    streams.swap(streams_);
    //
    //    PendingOutboundStreams pending_streams;
    //    pending_streams.swap(pending_outbound_streams_);
    //
    //    for (auto [_, stream] : streams) {
    //      stream->closedByConnection(notify_streams_code);
    //    }
    //
    //    for (auto [_, cb] : pending_streams) {
    //      cb(notify_streams_code);
    //    }
    //
    //    if (closed_callback_) {
    //      closed_callback_(remote_peer_, shared_from_this());
    //    }
  }

  //  void WsConnection::writeStreamData(uint32_t stream_id,
  //                                     gsl::span<const uint8_t> data, bool
  //                                     some) {
  //    if (some) {
  //      // header must be written not partially, even some == true
  //      enqueue(dataMsg(stream_id, data.size(), false));
  //      enqueue(Buffer(data.begin(), data.end()), stream_id, true);
  //    } else {
  //      // if !some then we can write a whole packet
  //      auto packet = dataMsg(stream_id, data.size(), true);
  //
  //      // will add support for vector writes some time
  //      packet.insert(packet.end(), data.begin(), data.end());
  //      enqueue(std::move(packet), stream_id);
  //    }
  //  }
  //
  //  void WsConnection::deferCall(std::function<void()> cb) {
  //    connection_->deferWriteCallback(std::error_code{},
  //                                    [cb = std::move(cb)](auto) { cb(); });
  //  }

  void WsConnection::enqueue(Buffer packet,
                             // StreamId stream_id,
                             bool some) {
    if (is_writing_) {
      write_queue_.push_back(WriteQueueItem{std::move(packet),
                                            // stream_id,
                                            some});
    } else {
      doWrite(WriteQueueItem{std::move(packet),
                             // stream_id,
                             some});
    }
  }

  void WsConnection::doWrite(WriteQueueItem packet) {
    assert(!is_writing_);

    //    auto write_func =
    //        packet.some ? &CapableConnection::writeSome :
    //        &CapableConnection::write;
    //    auto span = gsl::span<const uint8_t>(packet.packet);
    //    auto sz = packet.packet.size();
    //    auto cb = [wptr{weak_from_this()},
    //               packet = std::move(packet)](outcome::result<size_t> res) {
    //      auto self = wptr.lock();
    //      if (self)
    //        self->onDataWritten(res, packet.stream_id, packet.some);
    //    };
    //
    //    is_writing_ = true;
    //    ((connection_.get())->*write_func)(span, sz, std::move(cb));
  }

  void WsConnection::onDataWritten(outcome::result<size_t> res,
                                   // StreamId stream_id,
                                   bool some) {
    if (!res) {
      // write error
      close(res.error(), boost::none);
      return;
    }

    // this instance may be killed inside further callback
    auto wptr = weak_from_this();

    // if (stream_id != 0) {
    //   // pass write ack to stream about data size written except header size
    //
    //   auto sz = res.value();
    //   if (!some) {
    //     if (sz < WsFrame::kHeaderLength) {
    //       log_->error("onDataWritten : too small size arrived: {}", sz);
    //       sz = 0;
    //     } else {
    //       sz -= WsFrame::kHeaderLength;
    //     }
    //   }
    //
    //   // if (sz > 0) {
    //   //   auto it = streams_.find(stream_id);
    //   //   if (it == streams_.end()) {
    //   //     SL_DEBUG(log_, "onDataWritten : stream {} no longer exists",
    //   //              stream_id);
    //   //   } else {
    //   //     // stream can now call write callbacks
    //   //     it->second->onDataWritten(sz);
    //   //   }
    //   // }
    // }

    if (wptr.expired()) {
      // *this* no longer exists
      return;
    }

    is_writing_ = false;

    if (started_ && !write_queue_.empty()) {
      auto next_packet = std::move(write_queue_.front());
      write_queue_.pop_front();
      doWrite(std::move(next_packet));
    }
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
