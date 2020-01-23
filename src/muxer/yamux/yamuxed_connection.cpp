/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/asio/error.hpp>
#include <libp2p/muxer/yamux/yamuxed_connection.hpp>

#include <libp2p/muxer/yamux/yamux_frame.hpp>
#include <libp2p/muxer/yamux/yamux_stream.hpp>

using Buffer = libp2p::common::ByteArray;

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, YamuxedConnection::Error, e) {
  using ErrorType = libp2p::connection::YamuxedConnection::Error;
  switch (e) {
    case ErrorType::NO_SUCH_STREAM:
      return "no such stream was found; maybe, it is closed";
    case ErrorType::YAMUX_IS_CLOSED:
      return "this Yamux instance is closed";
    case ErrorType::TOO_MANY_STREAMS:
      return "streams number exceeded the maximum - close some of the existing "
             "in order to create a new one";
    case ErrorType::FORBIDDEN_CALL:
      return "forbidden method was invoked";
    case ErrorType::OTHER_SIDE_ERROR:
      return "error happened on other side's behalf";
    case ErrorType::INTERNAL_ERROR:
      return "internal error happened";
  }
  return "unknown";
}

namespace libp2p::connection {
  YamuxedConnection::YamuxedConnection(
      std::shared_ptr<SecureConnection> connection,
      muxer::MuxedConnectionConfig config)
      : header_buffer_(YamuxFrame::kHeaderLength, 0),
        data_buffer_(config.maximum_window_size, 0),
        connection_{std::move(connection)},
        config_{config} {
    // client uses odd numbers, server - even
    last_created_stream_id_ = connection_->isInitiator() ? 1 : 2;
  }

  void YamuxedConnection::start() {
    BOOST_ASSERT_MSG(!started_,
                     "YamuxedConnection already started (double start)");
    started_ = true;
    return doReadHeader();
  }

  void YamuxedConnection::stop() {
    BOOST_ASSERT_MSG(started_,
                     "YamuxedConnection is not started (double stop)");
    started_ = false;
  }

  void YamuxedConnection::newStream(StreamHandlerFunc cb) {
    BOOST_ASSERT_MSG(started_, "newStream is called but yamux is stopped");
    BOOST_ASSERT(config_.maximum_streams > 0);

    if (streams_.size() >= config_.maximum_streams) {
      return cb(Error::TOO_MANY_STREAMS);
    }

    auto stream_id = getNewStreamId();

    new_stream_pending_ = true;

    write(
        {newStreamMsg(stream_id),
         [self{shared_from_this()}, cb = std::move(cb), stream_id](auto &&res) {
           if (!res) {
             return cb(res.error());
           }
           auto created_stream =
               std::make_shared<YamuxStream>(self->weak_from_this(), stream_id,
                                             self->config_.maximum_window_size);
           self->streams_.insert({stream_id, created_stream});

           self->new_stream_pending_ = false;

           return cb(std::move(created_stream));
         }});
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
    started_ = false;
    resetAllStreams();
    streams_.clear();
    window_updates_subs_.clear();
    data_subs_.clear();
    return connection_->close();
  }

  bool YamuxedConnection::isClosed() const {
    return !started_ || connection_->isClosed();
  }

  void YamuxedConnection::read(gsl::span<uint8_t> out, size_t bytes,
                               ReadCallbackFunc cb) {
    connection_->read(out, bytes, std::move(cb));
  }

  void YamuxedConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                   ReadCallbackFunc cb) {
    connection_->readSome(out, bytes, std::move(cb));
  }

  void YamuxedConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                                WriteCallbackFunc cb) {
    connection_->write(in, bytes, std::move(cb));
  }

  void YamuxedConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                    WriteCallbackFunc cb) {
    connection_->writeSome(in, bytes, std::move(cb));
  }

  void YamuxedConnection::write(WriteData write_data) {
    write_queue_.push(std::move(write_data));
    if (is_writing_) {
      return;
    }
    is_writing_ = true;
    doWrite();
  }

  void YamuxedConnection::doWrite() {
    if (write_queue_.empty() || !started_ || connection_->isClosed()) {
      std::queue<WriteData>().swap(write_queue_);
      is_writing_ = false;
      return;
    }

    const auto &data = write_queue_.front();
    if (data.some) {
      return connection_->writeSome(
          data.data, data.data.size(), [self{shared_from_this()}](auto &&res) {
            self->writeCompleted(std::forward<decltype(res)>(res));
          });
    }
    return connection_->write(
        data.data, data.data.size(), [self{shared_from_this()}](auto &&res) {
          self->writeCompleted(std::forward<decltype(res)>(res));
        });
  }

  void YamuxedConnection::writeCompleted(outcome::result<size_t> res) {
    const auto &data = write_queue_.front();
    if (res) {
      data.cb(res.value() - YamuxFrame::kHeaderLength);
    } else {
      data.cb(std::forward<decltype(res)>(res));
    }
    write_queue_.pop();
    doWrite();
  }

  void YamuxedConnection::doReadHeader() {
    if (!started_ || connection_->isClosed()) {
      log_->info("connection was closed");
      return;
    }

    return connection_->read(
        header_buffer_, YamuxFrame::kHeaderLength,
        [self{shared_from_this()}](auto &&res) {
          self->readHeaderCompleted(std::forward<decltype(res)>(res));
        });
  }

  void YamuxedConnection::readHeaderCompleted(outcome::result<size_t> res) {
    using FrameType = YamuxFrame::FrameType;

    if (!res) {
      if (res.error().value() == boost::asio::error::eof) {
        log_->info("the client has closed a session");
        return;
      }
      log_->error(
          "cannot read header from the connection: {}; closing the session",
          res.error().message());
      return closeSession();
    }

    auto header_opt = parseFrame(header_buffer_);
    if (!header_opt) {
      log_->error(
          "client has sent something, which is not a valid header; closing the "
          "session");
      return closeSession();
    }

    switch (header_opt->type) {
      case FrameType::DATA:
      case FrameType::WINDOW_UPDATE:
        return processDataOrWindowUpdateFrame(*header_opt);
      case FrameType::PING:
        return processPingFrame(*header_opt);
      case FrameType::GO_AWAY:
        return processGoAwayFrame(*header_opt);
      default:
        log_->critical("garbage in parsed frame's type; closing the session");
        return closeSession();
    }
  }

  void YamuxedConnection::doReadData(size_t data_size,
                                     basic::Reader::ReadCallbackFunc cb) {
    // allocate enough memory
    data_buffer_.resize(data_size);
    // clear all previously stored data to prevent any unauthorized access
    std::fill(data_buffer_.begin(), data_buffer_.end(), 0u);
    /* memset could be faster than std::fill when compiler optimization is
     * disabled, but it had to operate with raw pointers that are discouraged.
     * Moreover, std::fill looks more idiomatic for that case */
    return connection_->read(
        data_buffer_, data_size,
        [self{shared_from_this()}, cb = std::move(cb)](auto &&res) {
          cb(std::forward<decltype(res)>(res));
        });
  }

  void YamuxedConnection::processDataOrWindowUpdateFrame(
      const YamuxFrame &frame) {
    using Flag = YamuxFrame::Flag;

    auto stream_id = frame.stream_id;
    auto stream = findStream(stream_id);

    // after the function execution decision to discard either data or window
    // update can be made
    auto discard = false;

    if (frame.flagIsSet(Flag::SYN)) {
      // request to open a new stream
      if (stream) {
        // duplicate stream request - critical protocol violation
        log_->error(
            "duplicate stream request was sent; closing the Yamux session");
        return closeSession();
      }

      if (streams_.size() < config_.maximum_streams && new_stream_handler_) {
        stream = registerNewStream(stream_id);
      } else {
        // if we cannot accept another stream, reset it on the other side
        write(
            {resetStreamMsg(stream_id), [self{shared_from_this()}](auto &&res) {
               if (!res) {
                 self->log_->error("cannot reset stream: {}",
                                   res.error().message());
               }
             }});
        discard = true;
      }
    }

    if (frame.flagIsSet(Flag::ACK)) {
      // ack of the stream we initiated
      if (!stream) {
        // if we don't have such a stream, reset it on the other side
        write(
            {resetStreamMsg(stream_id), [self{shared_from_this()}](auto &&res) {
               if (!res) {
                 self->log_->error("cannot reset stream: {}",
                                   res.error().message());
               }
             }});
        discard = true;
      }
    }

    if (frame.flagIsSet(Flag::FIN)) {
      closeStreamForRead(stream_id);
    }

    if (frame.flagIsSet(Flag::RST)) {
      removeStream(stream_id);
      discard = true;
    }

    if (frame.type == YamuxFrame::FrameType::DATA) {
      // even if the data is to be discarded, it still must be drawn from the
      // wire
      return processData(std::move(stream), frame, discard);
    }

    if (stream && !discard) {
      return processWindowUpdate(stream, frame.length);
    }

    doReadHeader();
  }

  void YamuxedConnection::processPingFrame(const YamuxFrame &frame) {
    write(
        {pingResponseMsg(frame.length), [self{shared_from_this()}](auto &&res) {
           if (!res) {
             self->log_->error("cannot write ping message: {}",
                               res.error().message());
           }
         }});
    doReadHeader();
  }

  void YamuxedConnection::resetAllStreams() {
    for (const auto &stream : streams_) {
      stream.second->resetStream();
    }
  }

  void YamuxedConnection::processGoAwayFrame(const YamuxFrame &frame) {
    started_ = false;
    resetAllStreams();
  }

  std::shared_ptr<YamuxStream> YamuxedConnection::findStream(
      StreamId stream_id) {
    auto stream = streams_.find(stream_id);
    if (stream == streams_.end()) {
      return nullptr;
    }
    return stream->second;
  }

  std::shared_ptr<YamuxStream> YamuxedConnection::registerNewStream(
      StreamId stream_id) {
    // optimistic approach: assuming ACK will be successfully written
    auto new_stream = std::make_shared<YamuxStream>(
        weak_from_this(), stream_id, config_.maximum_window_size);
    streams_.insert({stream_id, new_stream});
    new_stream_handler_(new_stream);

    write({ackStreamMsg(stream_id),
           [self{shared_from_this()}, stream_id](auto &&res) {
             if (!res) {
               self->log_->error("cannot register new stream: {}",
                                 res.error().message());
               self->removeStream(stream_id);
             }
           }});

    return new_stream;
  }

  void YamuxedConnection::processData(std::shared_ptr<YamuxStream> stream,
                                      const YamuxFrame &frame,
                                      bool discard_data) {
    auto data_len = frame.length;
    if (data_len == 0) {
      return doReadHeader();
    }

    if (data_len > config_.maximum_window_size) {
      log_->error(
          "too much data was received by this connection; closing the session");
      return closeSession();
    }

    // read the data, commit it to the stream and call handler, if exists
    doReadData(
        data_len,
        [self{shared_from_this()}, stream = std::move(stream), data_len, frame,
         discard_data](auto &&res) {
          if (!res) {
            self->log_->error("cannot read data from the connection: {}",
                              res.error().message());
            return self->closeSession();
          }

          if (stream && !discard_data) {
            auto commit_res = stream->commitData(self->data_buffer_, data_len);
            if (!commit_res) {
              self->log_->error("cannot commit data to the stream's buffer: {}",
                                commit_res.error().message());
              return self->closeSession();
            }
          } else {
            // the data is to be discarded
            return self->doReadHeader();
          }

          if (auto stream_data_sub = self->data_subs_.find(frame.stream_id);
              stream_data_sub != self->data_subs_.end()) {
            // if someone is waiting for the data from that stream, notify it
            if (stream_data_sub->second()) {
              self->data_subs_.erase(stream_data_sub);
            }
          }

          self->doReadHeader();
        });
  }

  void YamuxedConnection::processWindowUpdate(
      const std::shared_ptr<YamuxStream> &stream, uint32_t window_delta) {
    stream->send_window_size_ += window_delta;
    if (auto window_update_sub = window_updates_subs_.find(stream->stream_id_);
        window_update_sub != window_updates_subs_.end()) {
      if (window_update_sub->second()) {
        // if handler returns true, it means that it should be removed
        window_updates_subs_.erase(window_update_sub);
      }
    }
    doReadHeader();
  }

  void YamuxedConnection::closeStreamForRead(StreamId stream_id) {
    if (auto stream = findStream(stream_id)) {
      if (!stream->is_writable_) {
        removeStream(stream_id);
        return;
      }
      stream->is_readable_ = false;
    }
  }

  void YamuxedConnection::closeStreamForWrite(
      StreamId stream_id, std::function<void(outcome::result<void>)> cb) {
    if (auto stream = findStream(stream_id)) {
      return write({closeStreamMsg(stream_id),
                    [self{shared_from_this()}, cb = std::move(cb), stream_id,
                     stream](auto &&res) {
                      if (!res) {
                        self->log_->error(
                            "cannot close stream on the other side: {} ",
                            res.error().message());
                        return cb(res.error());
                      }
                      if (!stream->is_readable_) {
                        self->removeStream(stream_id);
                      } else {
                        stream->is_writable_ = false;
                      }
                      cb(outcome::success());
                    }});
    }
    return cb(Error::NO_SUCH_STREAM);
  }

  void YamuxedConnection::removeStream(StreamId stream_id) {
    if (auto stream = findStream(stream_id)) {
      streams_.erase(stream_id);
      stream->resetStream();

      // TODO(artem): temporarily cleanup itself!
      //      if (streams_.empty() && !new_stream_pending_) {
      //        auto res = close();
      //        if (!res) {
      //          log_->error("cannot close connection: {} ",
      //          res.error().message());
      //        }
      //      }
    }
  }

  YamuxedConnection::StreamId YamuxedConnection::getNewStreamId() {
    auto id = last_created_stream_id_;
    last_created_stream_id_ += 2;
    return id;
  }

  void YamuxedConnection::closeSession() {
    return write({goAwayMsg(YamuxFrame::GoAwayError::PROTOCOL_ERROR),
                  [self{shared_from_this()}](auto &&res) {
                    self->started_ = false;
                    if (!res) {
                      self->log_->error("cannot close a Yamux session: {} ",
                                        res.error().message());
                      return;
                    }
                    self->log_->info("Yamux session was closed");
                  }});
  }

  void YamuxedConnection::streamOnWindowUpdate(StreamId stream_id,
                                               NotifyeeCallback cb) {
    window_updates_subs_[stream_id] = std::move(cb);
  }

  void YamuxedConnection::streamOnAddData(StreamId stream_id,
                                          NotifyeeCallback cb) {
    data_subs_[stream_id] = std::move(cb);
  }

  void YamuxedConnection::streamWrite(StreamId stream_id,
                                      gsl::span<const uint8_t> in, size_t bytes,
                                      bool some,
                                      basic::Writer::WriteCallbackFunc cb) {
    if (!started_) {
      return cb(Error::YAMUX_IS_CLOSED);
    }

    if (auto stream = findStream(stream_id)) {
      return write({dataMsg(stream_id, Buffer{in.data(), in.data() + bytes}),
                    [self{shared_from_this()}, cb = std::move(cb)](auto &&res) {
                      if (!res) {
                        self->log_->error(
                            "cannot write data from the stream: {} ",
                            res.error().message());
                      }
                      return cb(std::forward<decltype(res)>(res));
                    },
                    some});
    }
    return cb(Error::NO_SUCH_STREAM);
  }

  void YamuxedConnection::streamAckBytes(
      StreamId stream_id, uint32_t bytes,
      std::function<void(outcome::result<void>)> cb) {
    if (auto stream = findStream(stream_id)) {
      return write({windowUpdateMsg(stream_id, bytes),
                    [self{shared_from_this()}, cb = std::move(cb)](auto &&res) {
                      if (!res) {
                        self->log_->error(
                            "cannot ack bytes from the stream: {} ",
                            res.error().message());
                        return cb(res.error());
                      }
                      cb(outcome::success());
                    }});
    }
    return cb(Error::NO_SUCH_STREAM);
  }

  void YamuxedConnection::streamClose(
      StreamId stream_id, std::function<void(outcome::result<void>)> cb) {
    if (auto stream = findStream(stream_id)) {
      return closeStreamForWrite(stream_id, std::move(cb));
    }
    return cb(Error::NO_SUCH_STREAM);
  }

  void YamuxedConnection::streamReset(
      StreamId stream_id, std::function<void(outcome::result<void>)> cb) {
    if (auto stream = findStream(stream_id)) {
      return write({resetStreamMsg(stream_id),
                    [self{shared_from_this()}, cb = std::move(cb),
                     stream_id](auto &&res) {
                      if (!res) {
                        self->log_->error("cannot reset stream: {} ",
                                          res.error().message());
                        return cb(res.error());
                      }
                      self->removeStream(stream_id);
                      cb(outcome::success());
                    }});
    }
    return cb(Error::NO_SUCH_STREAM);
  }

}  // namespace libp2p::connection
