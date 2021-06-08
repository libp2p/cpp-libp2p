/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamuxed_connection.hpp>

#include <boost/asio/error.hpp>

#include <libp2p/log/logger.hpp>

namespace libp2p::connection {

  namespace {
    const std::shared_ptr<soralog::Logger> &log() {
      static auto logger = log::createLogger("YamuxConn");
      return logger;
    }

    inline bool isOutbound(uint32_t our_stream_id, uint32_t their_stream_id) {
      // streams id oddness and evenness, depends on connection direction,
      // outbound or inbound, resp.
      return ((our_stream_id ^ their_stream_id) & 1) == 0;
    }

  }  // namespace

  YamuxedConnection::YamuxedConnection(
      std::shared_ptr<SecureConnection> connection,
      std::shared_ptr<basic::Scheduler> scheduler,
      ConnectionClosedCallback closed_callback,
      muxer::MuxedConnectionConfig config)
      : config_(config),
        connection_(std::move(connection)),
        scheduler_(std::move(scheduler)),
        raw_read_buffer_(std::make_shared<Buffer>()),
        reading_state_(
            [this](boost::optional<YamuxFrame> header) {
              return processHeader(std::move(header));
            },
            [this](gsl::span<uint8_t> segment, StreamId stream_id, bool rst,
                   bool fin) {
              if (!segment.empty()) {
                processData(segment, stream_id);
              }
              if (rst) {
                processRst(stream_id);
              }
              if (fin) {
                processFin(stream_id);
              }
            }),
        closed_callback_(std::move(closed_callback)),

        // yes, sort of assert
        remote_peer_(std::move(connection_->remotePeer().value())) {
    assert(scheduler_);
    assert(config_.maximum_streams > 0);
    assert(config_.maximum_window_size >= YamuxFrame::kInitialWindowSize);

    raw_read_buffer_->resize(YamuxFrame::kInitialWindowSize + 4096);
    new_stream_id_ = (connection_->isInitiator() ? 1 : 2);
  }

  void YamuxedConnection::start() {
    if (started_) {
      log()->error("already started (double start)");
      return;
    }
    started_ = true;

    static constexpr auto kCleanupInterval = std::chrono::seconds(150);
    cleanup_handle_ = scheduler_->scheduleWithHandle(
        [this]() {
          if (started_) {
            std::vector<StreamId> abandoned;
            for (auto &[id, stream] : streams_) {
              if (stream.use_count() == 1) {
                abandoned.push_back(id);
                enqueue(resetStreamMsg(id));
              }
            }
            if (!abandoned.empty()) {
              log()->info("cleaning up {} abandoned streams", abandoned.size());
              for (const auto id : abandoned) {
                streams_.erase(id);
              }
            }
            std::ignore = cleanup_handle_.reschedule(kCleanupInterval);
          }
        },
        kCleanupInterval);

    if (config_.ping_interval != std::chrono::milliseconds::zero()) {
      ping_handle_ = scheduler_->scheduleWithHandle(
          [this, ping_counter = 0u]() mutable {
            if (started_) {
              // dont send pings if something is being written
              if (!is_writing_) {
                enqueue(pingOutMsg(++ping_counter));
                SL_TRACE(log(), "written ping message #{}", ping_counter);
              }
              std::ignore = ping_handle_.reschedule(config_.ping_interval);
            }
          },
          config_.ping_interval);
    }

    continueReading();
  }

  void YamuxedConnection::stop() {
    if (!started_) {
      log()->error("already stopped (double stop)");
      return;
    }
    started_ = false;
  }

  outcome::result<std::shared_ptr<Stream>> YamuxedConnection::newStream() {
    if (!started_) {
      return Error::CONNECTION_NOT_ACTIVE;
    }

    if (streams_.size() >= config_.maximum_streams) {
      return Error::CONNECTION_TOO_MANY_STREAMS;
    }

    auto stream_id = new_stream_id_;
    new_stream_id_ += 2;
    enqueue(newStreamMsg(stream_id));

    // Now we self-acked the new stream
    return createStream(stream_id);
  }

  void YamuxedConnection::newStream(StreamHandlerFunc cb) {
    if (!started_) {
      return connection_->deferWriteCallback(
          Error::CONNECTION_NOT_ACTIVE,
          [cb = std::move(cb)](auto) { cb(Error::CONNECTION_NOT_ACTIVE); });
    }

    if (streams_.size() >= config_.maximum_streams) {
      return connection_->deferWriteCallback(
          Error::CONNECTION_TOO_MANY_STREAMS, [cb = std::move(cb)](auto) {
            cb(Error::CONNECTION_TOO_MANY_STREAMS);
          });
    }

    auto stream_id = new_stream_id_;
    new_stream_id_ += 2;
    enqueue(newStreamMsg(stream_id));
    pending_outbound_streams_[stream_id] = std::move(cb);
    inactivity_handle_.cancel();
  }

  void YamuxedConnection::onStream(NewStreamHandlerFunc cb) {
    new_stream_handler_ = std::move(cb);
  }

  outcome::result<peer::PeerId> YamuxedConnection::localPeer() const {
    return connection_->localPeer();
  }

  outcome::result<peer::PeerId> YamuxedConnection::remotePeer() const {
    return remote_peer_;
  }

  outcome::result<crypto::PublicKey> YamuxedConnection::remotePublicKey()
      const {
    return connection_->remotePublicKey();
  }

  bool YamuxedConnection::isInitiator() const noexcept {
    return connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> YamuxedConnection::localMultiaddr() {
    return connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> YamuxedConnection::remoteMultiaddr() {
    return connection_->remoteMultiaddr();
  }

  outcome::result<void> YamuxedConnection::close() {
    close(Error::CONNECTION_CLOSED_BY_HOST, YamuxFrame::GoAwayError::NORMAL);
    return outcome::success();
  }

  bool YamuxedConnection::isClosed() const {
    return !started_ || connection_->isClosed();
  }

  void YamuxedConnection::read(gsl::span<uint8_t> out, size_t bytes,
                               ReadCallbackFunc cb) {
    log()->error("YamuxedConnection::read : invalid direct call");
    deferReadCallback(Error::CONNECTION_DIRECT_IO_FORBIDDEN, std::move(cb));
  }

  void YamuxedConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                   ReadCallbackFunc cb) {
    log()->error("YamuxedConnection::readSome : invalid direct call");
    deferReadCallback(Error::CONNECTION_DIRECT_IO_FORBIDDEN, std::move(cb));
  }

  void YamuxedConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                                WriteCallbackFunc cb) {
    log()->error("YamuxedConnection::write : invalid direct call");
    deferWriteCallback(Error::CONNECTION_DIRECT_IO_FORBIDDEN, std::move(cb));
  }

  void YamuxedConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                    WriteCallbackFunc cb) {
    log()->error("YamuxedConnection::writeSome : invalid direct call");
    deferWriteCallback(Error::CONNECTION_DIRECT_IO_FORBIDDEN, std::move(cb));
  }

  void YamuxedConnection::deferReadCallback(outcome::result<size_t> res,
                                            ReadCallbackFunc cb) {
    connection_->deferReadCallback(res, std::move(cb));
  }

  void YamuxedConnection::deferWriteCallback(std::error_code ec,
                                             WriteCallbackFunc cb) {
    connection_->deferWriteCallback(ec, std::move(cb));
  }

  void YamuxedConnection::continueReading() {
    SL_TRACE(log(), "YamuxedConnection::continueReading");
    connection_->readSome(*raw_read_buffer_, raw_read_buffer_->size(),
                          [wptr = weak_from_this(), buffer = raw_read_buffer_](
                              outcome::result<size_t> res) {
                            auto self = wptr.lock();
                            if (self) {
                              self->onRead(res);
                            }
                          });
  }

  void YamuxedConnection::onRead(outcome::result<size_t> res) {
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

    SL_TRACE(log(), "read {} bytes", n);

    assert(n <= raw_read_buffer_->size());

    if (n < raw_read_buffer_->size()) {
      bytes_read = bytes_read.first(ssize_t(n));
    }

    reading_state_.onDataReceived(bytes_read);

    if (!started_) {
      return;
    }

    std::vector<std::pair<StreamId, StreamHandlerFunc>> streams_created;
    streams_created.swap(fresh_streams_);
    for (const auto &[id, handler] : streams_created) {
      auto it = streams_.find(id);

      assert(it != streams_.end());

      if (it == streams_.end()) {
        log()->error("fresh_streams_ inconsistency!");
        continue;
      }

      auto stream = it->second;

      if (!handler) {
        // inbound
        assert(!isOutbound(new_stream_id_, id));
        assert(new_stream_handler_);

        new_stream_handler_(std::move(stream));
      } else {
        handler(std::move(stream));
      }

      if (!started_) {
        return;
      }
    }

    continueReading();
  }

  bool YamuxedConnection::processHeader(boost::optional<YamuxFrame> header) {
    using FrameType = YamuxFrame::FrameType;

    if (!header) {
      SL_DEBUG(log(), "cannot parse yamux frame: corrupted");
      close(Error::CONNECTION_PROTOCOL_ERROR,
            YamuxFrame::GoAwayError::PROTOCOL_ERROR);
      return false;
    }

    SL_TRACE(log(), "YamuxedConnection::processHeader");

    auto &frame = header.value();

    if (frame.type == FrameType::GO_AWAY) {
      processGoAway(frame);
      return false;
    }

    bool is_rst = frame.flagIsSet(YamuxFrame::Flag::RST);
    bool is_fin = frame.flagIsSet(YamuxFrame::Flag::FIN);
    bool is_ack = frame.flagIsSet(YamuxFrame::Flag::ACK);
    bool is_syn = frame.flagIsSet(YamuxFrame::Flag::SYN);

    // new inbound stream or ping
    if (is_syn && !processSyn(frame)) {
      return false;
    }

    // outbound stream accepted or pong
    if (is_ack && !processAck(frame)) {
      return false;
    }

    // increase window size
    if (frame.type == FrameType::WINDOW_UPDATE && !processWindowUpdate(frame)) {
      return false;
    }

    if (is_fin && (frame.stream_id != 0)) {
      processFin(frame.stream_id);
    }

    if (is_rst && (frame.stream_id != 0)) {
      processRst(frame.stream_id);
    }

    // proceed with incoming data
    return true;
  }

  void YamuxedConnection::processData(gsl::span<uint8_t> segment,
                                      StreamId stream_id) {
    assert(stream_id != 0);
    assert(!segment.empty());

    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
      // this may be due to overflow in previous fragments of same message
      SL_DEBUG(log(), "stream {} no longer exists", stream_id);
      reading_state_.discardDataMessage();
      return;
    }

    SL_TRACE(log(), "YamuxedConnection::processData, stream={}, size={}",
             stream_id, segment.size());

    auto result = it->second->onDataReceived(segment);
    if (result == YamuxStream::kKeepStream) {
      return;
    }

    eraseStream(stream_id);
    reading_state_.discardDataMessage();

    if (result == YamuxStream::kRemoveStreamAndSendRst) {
      // overflow, reset this stream
      enqueue(resetStreamMsg(stream_id));
    }
  }

  void YamuxedConnection::processGoAway(const YamuxFrame &frame) {
    SL_DEBUG(log(), "closed by remote peer, code={}", frame.length);
    close(Error::CONNECTION_CLOSED_BY_PEER, boost::none);
  }

  bool YamuxedConnection::processSyn(const YamuxFrame &frame) {
    bool ok = true;

    if (frame.stream_id == 0) {
      if (frame.type == YamuxFrame::FrameType::PING) {
        enqueue(pingResponseMsg(frame.length));
        return true;
      }
      SL_DEBUG(log(), "received SYN on zero stream id");
      ok = false;

    } else if (isOutbound(new_stream_id_, frame.stream_id)) {
      SL_DEBUG(log(), "received SYN with stream id of wrong direction");
      ok = false;

    } else if (streams_.count(frame.stream_id) != 0) {
      SL_DEBUG(log(), "received SYN on existing stream id");
      ok = false;

    } else if (streams_.size() + pending_outbound_streams_.size()
               > config_.maximum_streams) {
      SL_DEBUG(
          log(),
          "maximum number of streams ({}) exceeded, ignoring inbound stream",
          config_.maximum_streams);
      // if we cannot accept another stream, reset it on the other side
      enqueue(resetStreamMsg(frame.stream_id));
      return true;

    } else if (!new_stream_handler_) {
      log()->error("new stream handler not set");
      close(Error::CONNECTION_INTERNAL_ERROR,
            YamuxFrame::GoAwayError::INTERNAL_ERROR);
      return false;
    }

    if (!ok) {
      close(Error::CONNECTION_PROTOCOL_ERROR,
            YamuxFrame::GoAwayError::PROTOCOL_ERROR);
      return false;
    }

    SL_DEBUG(log(), "creating inbound stream {}", frame.stream_id);
    std::ignore = createStream(frame.stream_id);

    enqueue(ackStreamMsg(frame.stream_id));

    // handler will be called after all inbound bytes processed
    fresh_streams_.push_back({frame.stream_id, StreamHandlerFunc{}});

    return true;
  }

  bool YamuxedConnection::processAck(const YamuxFrame &frame) {
    bool ok = true;

    StreamHandlerFunc stream_handler;

    if (frame.stream_id == 0) {
      if (frame.type != YamuxFrame::FrameType::PING) {
        SL_DEBUG(log(), "received ACK on zero stream id");
        ok = false;
      } else {
        // pong has come. TODO(artem): measure latency
        return true;
      }

    } else {
      auto it = pending_outbound_streams_.find(frame.stream_id);
      if (it == pending_outbound_streams_.end()) {
        if (streams_.count(frame.stream_id) != 0) {
          // Stream was opened in optimistic manner
          SL_DEBUG(log(), "ignoring received ACK on existing stream id {}",
                   frame.stream_id);
          return true;
        }
        SL_DEBUG(log(), "received ACK on unknown stream id {}",
                 frame.stream_id);
        ok = false;
      }
      stream_handler = std::move(it->second);
      erasePendingOutboundStream(it);
    }

    if (!ok) {
      close(Error::CONNECTION_PROTOCOL_ERROR,
            YamuxFrame::GoAwayError::PROTOCOL_ERROR);
      return false;
    }

    assert(stream_handler);

    SL_DEBUG(log(), "creating outbound stream {}", frame.stream_id);

    std::ignore = createStream(frame.stream_id);

    // handler will be called after all inbound bytes processed
    fresh_streams_.emplace_back(frame.stream_id, std::move(stream_handler));

    return true;
  }

  void YamuxedConnection::processFin(StreamId stream_id) {
    assert(stream_id != 0);

    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
      if (isOutbound(new_stream_id_, stream_id)) {
        // almost not probable
        auto it2 = pending_outbound_streams_.find(stream_id);
        if (it2 != pending_outbound_streams_.end()) {
          SL_DEBUG(log(), "received FIN to pending outbound stream {}",
                   stream_id);
          auto cb = std::move(it2->second);
          erasePendingOutboundStream(it2);
          cb(Stream::Error::STREAM_RESET_BY_PEER);
          return;
        }
      }
      SL_DEBUG(log(), "stream {} no longer exists", stream_id);
      return;
    }

    auto result = it->second->onFINReceived();
    if (result == YamuxStream::kRemoveStream) {
      eraseStream(stream_id);
    }

    return;
  }

  void YamuxedConnection::processRst(StreamId stream_id) {
    assert(stream_id != 0);

    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
      if (isOutbound(new_stream_id_, stream_id)) {
        auto it2 = pending_outbound_streams_.find(stream_id);
        if (it2 != pending_outbound_streams_.end()) {
          SL_DEBUG(log(), "received RST to pending outbound stream {}",
                   stream_id);

          auto cb = std::move(it2->second);
          erasePendingOutboundStream(it2);
          cb(Stream::Error::STREAM_RESET_BY_PEER);
          return;
        }
      }

      SL_DEBUG(log(), "stream {} no longer exists", stream_id);
      return;
    }

    auto stream = std::move(it->second);
    eraseStream(stream_id);
    stream->onRSTReceived();
  }

  bool YamuxedConnection::processWindowUpdate(const YamuxFrame &frame) {
    auto it = streams_.find(frame.stream_id);
    if (it != streams_.end()) {
      it->second->increaseSendWindow(frame.length);
    } else {
      SL_DEBUG(log(), "processWindowUpdate: stream {} not found",
               frame.stream_id);
    }

    return true;
  }

  void YamuxedConnection::close(
      std::error_code notify_streams_code,
      boost::optional<YamuxFrame::GoAwayError> reply_to_peer_code) {
    if (!started_) {
      return;
    }

    started_ = false;

    SL_DEBUG(log(), "closing connection, reason: {}",
             notify_streams_code.message());

    write_queue_.clear();

    if (reply_to_peer_code.has_value() && !connection_->isClosed()) {
      enqueue(goAwayMsg(reply_to_peer_code.value()));
    }

    Streams streams;
    streams.swap(streams_);

    PendingOutboundStreams pending_streams;
    pending_streams.swap(pending_outbound_streams_);

    for (auto [_, stream] : streams) {
      stream->closedByConnection(notify_streams_code);
    }

    for (auto [_, cb] : pending_streams) {
      cb(notify_streams_code);
    }

    if (closed_callback_) {
      closed_callback_(remote_peer_, shared_from_this());
    }
  }

  void YamuxedConnection::writeStreamData(uint32_t stream_id,
                                          gsl::span<const uint8_t> data,
                                          bool some) {
    if (some) {
      // header must be written not partially, even some == true
      enqueue(dataMsg(stream_id, data.size(), false));
      enqueue(Buffer(data.begin(), data.end()), stream_id, true);
    } else {
      // if !some then we can write a whole packet
      auto packet = dataMsg(stream_id, data.size(), true);

      // will add support for vector writes some time
      packet.insert(packet.end(), data.begin(), data.end());
      enqueue(std::move(packet), stream_id);
    }
  }

  void YamuxedConnection::ackReceivedBytes(uint32_t stream_id, uint32_t bytes) {
    enqueue(windowUpdateMsg(stream_id, bytes));
  }

  void YamuxedConnection::deferCall(std::function<void()> cb) {
    connection_->deferWriteCallback(std::error_code{},
                                    [cb = std::move(cb)](auto) { cb(); });
  }

  void YamuxedConnection::resetStream(StreamId stream_id) {
    SL_DEBUG(log(), "RST from stream {}", stream_id);
    enqueue(resetStreamMsg(stream_id));
    eraseStream(stream_id);
  }

  void YamuxedConnection::streamClosed(uint32_t stream_id) {
    // send FIN and reset stream only if other side has closed this way

    SL_DEBUG(log(), "sending FIN to stream {}", stream_id);

    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
      log()->error("YamuxedConnection::streamClosed: stream {} not found",
                   stream_id);
      return;
    }

    enqueue(closeStreamMsg(stream_id));

    auto &stream = it->second;
    assert(stream->isClosedForWrite());

    if (stream->isClosedForRead()) {
      eraseStream(stream_id);
    }
  }

  void YamuxedConnection::enqueue(Buffer packet, StreamId stream_id,
                                  bool some) {
    if (is_writing_) {
      write_queue_.push_back(
          WriteQueueItem{std::move(packet), stream_id, some});
    } else {
      doWrite(WriteQueueItem{std::move(packet), stream_id, some});
    }
  }

  void YamuxedConnection::doWrite(WriteQueueItem packet) {
    assert(!is_writing_);

    auto write_func =
        packet.some ? &CapableConnection::writeSome : &CapableConnection::write;
    auto span = gsl::span<const uint8_t>(packet.packet);
    auto sz = packet.packet.size();
    auto cb = [wptr{weak_from_this()},
               packet = std::move(packet)](outcome::result<size_t> res) {
      auto self = wptr.lock();
      if (self)
        self->onDataWritten(res, packet.stream_id, packet.some);
    };

    is_writing_ = true;
    ((connection_.get())->*write_func)(span, sz, std::move(cb));
  }

  void YamuxedConnection::onDataWritten(outcome::result<size_t> res,
                                        StreamId stream_id, bool some) {
    if (!res) {
      // write error
      close(res.error(), boost::none);
      return;
    }

    // this instance may be killed inside further callback
    auto wptr = weak_from_this();

    if (stream_id != 0) {
      // pass write ack to stream about data size written except header size

      auto sz = res.value();
      if (!some) {
        if (sz < YamuxFrame::kHeaderLength) {
          log()->error("onDataWritten : too small size arrived: {}", sz);
          sz = 0;
        } else {
          sz -= YamuxFrame::kHeaderLength;
        }
      }

      if (sz > 0) {
        auto it = streams_.find(stream_id);
        if (it == streams_.end()) {
          SL_DEBUG(log(), "onDataWritten : stream {} no longer exists",
                   stream_id);
        } else {
          // stream can now call write callbacks
          it->second->onDataWritten(sz);
        }
      }
    }

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

  std::shared_ptr<Stream> YamuxedConnection::createStream(StreamId stream_id) {
    auto stream = std::make_shared<YamuxStream>(
        shared_from_this(), *this, stream_id, config_.maximum_window_size,
        basic::WriteQueue::kDefaultSizeLimit);
    streams_[stream_id] = stream;
    inactivity_handle_.cancel();
    return stream;
  }

  void YamuxedConnection::eraseStream(StreamId stream_id) {
    SL_DEBUG(log(), "erasing stream {}", stream_id);
    streams_.erase(stream_id);
    adjustExpireTimer();
  }

  void YamuxedConnection::erasePendingOutboundStream(
      PendingOutboundStreams::iterator it) {
    SL_TRACE(log(), "erasing pending outbound stream {}", it->first);
    pending_outbound_streams_.erase(it);
    adjustExpireTimer();
  }

  void YamuxedConnection::adjustExpireTimer() {
    if (config_.no_streams_interval > basic::kZeroTime && streams_.empty()
        && pending_outbound_streams_.empty()) {
      SL_DEBUG(log(), "scheduling expire timer to {} msec",
               config_.no_streams_interval.count());
      inactivity_handle_ = scheduler_->scheduleWithHandle(
          [this] { onExpireTimer(); },
          scheduler_->now() + config_.no_streams_interval);
    }
  }

  void YamuxedConnection::onExpireTimer() {
    if (streams_.empty() && pending_outbound_streams_.empty()) {
      SL_DEBUG(log(), "closing expired connection");
      close(Error::CONNECTION_NOT_ACTIVE, YamuxFrame::GoAwayError::NORMAL);
    }
  }

}  // namespace libp2p::connection
