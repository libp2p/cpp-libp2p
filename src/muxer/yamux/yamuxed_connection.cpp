/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamuxed_connection.hpp>

#include <boost/asio/error.hpp>

#include <libp2p/muxer/yamux/yamux_error.hpp>

#define TRACE_ENABLED 1
#include <libp2p/common/trace.hpp>

namespace libp2p::connection {

  namespace {
    auto log() {
      static auto logger = libp2p::log::createLogger("YamuxConn");
      return logger.get();
    }

    inline size_t size(const gsl::span<uint8_t> &span) {
      return static_cast<size_t>(span.size());
    }

    inline std::tuple<gsl::span<uint8_t>, gsl::span<uint8_t>> split(
        gsl::span<uint8_t> span, size_t n) {
      return {span.first(n), span.subspan(n)};
    }

    inline bool isOutbound(uint32_t our_stream_id, uint32_t their_stream_id) {
      // streams id oddness and evenness, depends on connection direction,
      // outbound or inbound, resp.
      return ((our_stream_id ^ their_stream_id) & 1) == 0;
    }

  }  // namespace

  YamuxedConnection::YamuxedConnection(
      std::shared_ptr<SecureConnection> connection,
      muxer::MuxedConnectionConfig config)
      : config_(config),
        connection_(std::move(connection)),
        raw_read_buffer_(std::make_shared<Buffer>()),
        reading_state_(
            [this](boost::optional<YamuxFrame> header) {
              return processHeader(std::move(header));
            },
            [this](gsl::span<uint8_t> segment, StreamId stream_id, bool rst,
                   bool fin) {
              if (!segment.empty()) {
                if (!processData(segment, stream_id)) {
                  return false;
                }
              }
              if (rst) {
                return processRst(stream_id);
              }
              if (fin) {
                return processFin(stream_id);
              }
              return true;
            }) {
    assert(connection_);
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
    continueReading();
  }

  void YamuxedConnection::stop() {
    if (!started_) {
      log()->error("already stopped (double stop)");
      return;
    }
    started_ = false;
  }

  void YamuxedConnection::newStream(StreamHandlerFunc cb) {
    if (!started_) {
      return connection_->deferWriteCallback(
          YamuxError::CONNECTION_STOPPED,
          [cb = std::move(cb)](auto) { cb(YamuxError::CONNECTION_STOPPED); });
    }

    if (streams_.size() >= config_.maximum_streams) {
      return connection_->deferWriteCallback(
          YamuxError::TOO_MANY_STREAMS,
          [cb = std::move(cb)](auto) { cb(YamuxError::TOO_MANY_STREAMS); });
    }

    auto stream_id = new_stream_id_;
    new_stream_id_ += 2;
    enqueue(newStreamMsg(stream_id));
    pending_outbound_streams_[stream_id] = std::move(cb);
  }

  void YamuxedConnection::onStream(NewStreamHandlerFunc cb) {
    new_stream_handler_ = std::move(cb);
  }

  outcome::result<peer::PeerId> YamuxedConnection::localPeer() const {
    return connection_->localPeer();
  }

  outcome::result<peer::PeerId> YamuxedConnection::remotePeer() const {
    return connection_->remotePeer();
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
    close(YamuxError::CONNECTION_CLOSED_BY_HOST,
          YamuxFrame::GoAwayError::NORMAL);
    return outcome::success();
  }

  bool YamuxedConnection::isClosed() const {
    return !started_ || connection_->isClosed();
  }

  void YamuxedConnection::read(gsl::span<uint8_t> out, size_t bytes,
                               ReadCallbackFunc cb) {
    log()->critical("YamuxedConnection::read : invalid direct call");
    deferReadCallback(YamuxError::FORBIDDEN_CALL, std::move(cb));
  }

  void YamuxedConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                   ReadCallbackFunc cb) {
    log()->critical("YamuxedConnection::readSome : invalid direct call");
    deferReadCallback(YamuxError::FORBIDDEN_CALL, std::move(cb));
  }

  void YamuxedConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                                WriteCallbackFunc cb) {
    log()->critical("YamuxedConnection::write : invalid direct call");
    deferWriteCallback(YamuxError::FORBIDDEN_CALL, std::move(cb));
  }

  void YamuxedConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                    WriteCallbackFunc cb) {
    log()->critical("YamuxedConnection::writeSome : invalid direct call");
    deferWriteCallback(YamuxError::FORBIDDEN_CALL, std::move(cb));
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
    TRACE("YamuxedConnection::continueReading");
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
        ec = YamuxError::CONNECTION_CLOSED_BY_PEER;
      }
      close(ec, boost::none);
      return;
    }

    auto n = res.value();
    gsl::span<uint8_t> bytes_read(*raw_read_buffer_);

    TRACE("read {} bytes", n);

    assert(n <= raw_read_buffer_->size());

    if (n < raw_read_buffer_->size()) {
      bytes_read = bytes_read.first(n);
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
        log()->critical("fresh_streams_ inconsistency!");
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
      log()->debug("cannot parse yamux frame: corrupted");
      close(YamuxError::PROTOCOL_ERROR,
            YamuxFrame::GoAwayError::PROTOCOL_ERROR);
      return false;
    }

    TRACE("YamuxedConnection::processHeader");

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

    if (is_fin && (frame.stream_id != 0) && !processFin(frame.stream_id)) {
      return false;
    }

    if (is_rst && (frame.stream_id != 0) && !processRst(frame.stream_id)) {
      return false;
    }

    // proceed with incoming data
    return true;
  }

  bool YamuxedConnection::processData(gsl::span<uint8_t> segment,
                                      StreamId stream_id) {
    assert(stream_id != 0);
    assert(!segment.empty());

    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
      // this may be due to overflow in previous fragments of same message
      log()->debug("stream {} no longer exists", stream_id);
      reading_state_.discardDataMessage();
      return true;
    }

    TRACE("YamuxedConnection::processData, stream={}, size={}", stream_id,
          segment.size());

    auto result = it->second->onDataReceived(segment);
    if (result == YamuxStream::kKeepStream) {
      return true;
    }

    eraseStream(stream_id);
    reading_state_.discardDataMessage();

    if (result == YamuxStream::kRemoveStreamAndSendRst) {
      // overflow, reset this stream
      enqueue(resetStreamMsg(stream_id));
    }
    return true;
  }

  void YamuxedConnection::processGoAway(const YamuxFrame &frame) {
    log()->debug("closed by remote peer, code={}", frame.length);
    close(YamuxError::CONNECTION_CLOSED_BY_PEER, boost::none);
  }

  bool YamuxedConnection::processSyn(const YamuxFrame &frame) {
    bool ok = true;

    if (frame.stream_id == 0) {
      if (frame.type == YamuxFrame::FrameType::PING) {
        enqueue(pingResponseMsg(frame.length));
        return true;
      }
      log()->debug("received SYN on zero stream id");
      ok = false;

    } else if (isOutbound(new_stream_id_, frame.stream_id)) {
      log()->debug("received SYN with stream id of wrong direction");
      ok = false;

    } else if (streams_.count(frame.stream_id) != 0) {
      log()->debug("received SYN on existing stream id");
      ok = false;

    } else if (streams_.size() + pending_outbound_streams_.size()
               > config_.maximum_streams) {
      log()->debug(
          "maximum number of streams ({}) exceeded, ignoring inbound stream");
      // if we cannot accept another stream, reset it on the other side
      enqueue(resetStreamMsg(frame.stream_id));
      return true;

    } else if (!new_stream_handler_) {
      log()->critical("new stream handler not set");
      close(YamuxError::INTERNAL_ERROR,
            YamuxFrame::GoAwayError::INTERNAL_ERROR);
      return false;
    }

    if (!ok) {
      close(YamuxError::PROTOCOL_ERROR,
            YamuxFrame::GoAwayError::PROTOCOL_ERROR);
      return false;
    }

    log()->debug("creating inbound stream {}", frame.stream_id);

    // create new stream
    streams_[frame.stream_id] = std::make_shared<YamuxStream>(
        shared_from_this(), *this, frame.stream_id, config_.maximum_window_size,
        basic::WriteQueue::kDefaultSizeLimit);

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
        log()->debug("received ACK on zero stream id");
        ok = false;
      } else {
        // pong has come. TODO(artem): measure latency
        return true;
      }

    } else if (streams_.count(frame.stream_id) != 0) {
      log()->debug("received ACK on existing stream id");
      ok = false;
    } else {
      auto it = pending_outbound_streams_.find(frame.stream_id);
      if (it == pending_outbound_streams_.end()) {
        log()->debug("received ACK on unknown stream id");
        ok = false;
      }
      stream_handler = std::move(it->second);
      pending_outbound_streams_.erase(it);
    }

    if (!ok) {
      close(YamuxError::PROTOCOL_ERROR,
            YamuxFrame::GoAwayError::PROTOCOL_ERROR);
      return false;
    }

    assert(stream_handler);

    log()->debug("creating outbound stream {}", frame.stream_id);

    streams_[frame.stream_id] = std::make_shared<YamuxStream>(
        shared_from_this(), *this, frame.stream_id, config_.maximum_window_size,
        basic::WriteQueue::kDefaultSizeLimit);

    // handler will be called after all inbound bytes processed
    fresh_streams_.emplace_back(frame.stream_id, std::move(stream_handler));

    return true;
  }

  bool YamuxedConnection::processFin(StreamId stream_id) {
    assert(stream_id != 0);

    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
      if (isOutbound(new_stream_id_, stream_id)) {
        // almost not probable
        auto it2 = pending_outbound_streams_.find(stream_id);
        if (it2 != pending_outbound_streams_.end()) {
          log()->debug("received FIN to pending outbound stream {}", stream_id);
          auto cb = std::move(it2->second);
          pending_outbound_streams_.erase(it2);
          cb(YamuxError::STREAM_RESET_BY_PEER);
          return true;
        }
      }
      log()->debug("stream {} no longer exists", stream_id);
      return true;
    }

    auto result = it->second->onFINReceived();
    if (result == YamuxStream::kRemoveStream) {
      eraseStream(stream_id);
    }

    return true;
  }

  bool YamuxedConnection::processRst(StreamId stream_id) {
    assert(stream_id != 0);

    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
      if (isOutbound(new_stream_id_, stream_id)) {
        auto it2 = pending_outbound_streams_.find(stream_id);
        if (it2 != pending_outbound_streams_.end()) {
          log()->debug("received RST to pending outbound stream {}", stream_id);

          auto cb = std::move(it2->second);
          pending_outbound_streams_.erase(it2);
          cb(YamuxError::STREAM_RESET_BY_PEER);
          return true;
        }
      }

      log()->debug("stream {} no longer exists", stream_id);
      return true;
    }

    auto stream = std::move(it->second);
    eraseStream(stream_id);
    stream->onRSTReceived();
    return true;
  }

  bool YamuxedConnection::processWindowUpdate(const YamuxFrame &frame) {
    auto it = streams_.find(frame.stream_id);
    if (it != streams_.end()) {
      it->second->increaseSendWindow(frame.length);
    } else {
      log()->debug("processWindowUpdate: stream {} not found", frame.stream_id);
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

    // TODO (artem) close and message bus

    log()->debug("closing connection, reason: {}",
                 notify_streams_code.message());

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

    if (reply_to_peer_code.has_value()) {
      enqueue(goAwayMsg(reply_to_peer_code.value()));
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
    log()->debug("RST from stream {}", stream_id);
    enqueue(resetStreamMsg(stream_id));
    eraseStream(stream_id);
  }

  void YamuxedConnection::streamClosed(uint32_t stream_id) {
    // send FIN and reset stream only if other side has closed this way

    log()->debug("sending FIN to stream {}", stream_id);

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
          log()->debug("onDataWritten : stream {} no longer exists", stream_id);
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

  void YamuxedConnection::eraseStream(StreamId stream_id) {
    log()->debug("erasing stream {}", stream_id);
    streams_.erase(stream_id);
  }

}  // namespace libp2p::connection
