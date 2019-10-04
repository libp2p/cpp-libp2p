/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/mplex/mplexed_connection.hpp>

#include <boost/assert.hpp>
#include <libp2p/muxer/mplex/mplex_error.hpp>
#include <libp2p/muxer/mplex/mplex_frame.hpp>
#include <libp2p/muxer/mplex/mplex_stream.hpp>

namespace libp2p::connection {
  MplexedConnection::MplexedConnection(
      std::shared_ptr<SecureConnection> connection,
      muxer::MuxedConnectionConfig config)
      : connection_{std::move(connection)}, config_{config} {
    BOOST_ASSERT(connection_);
  }

  void MplexedConnection::start() {
    BOOST_ASSERT_MSG(!is_active_,
                     "trying to start an active MplexedConnection");
    is_active_ = true;
    log_->info("starting an mplex connection");
    readNextFrame();
  }

  void MplexedConnection::stop() {
    BOOST_ASSERT_MSG(is_active_,
                     "trying to stop an inactive MplexedConnection");
    is_active_ = false;
    log_->info("stopping an mplex connection");
  }

  void MplexedConnection::newStream(StreamHandlerFunc cb) {
    BOOST_ASSERT_MSG(
        is_active_,
        "trying to open a stream over an inactive MplexedConnection");

    if (streams_.size() >= config_.maximum_streams) {
      return cb(MplexError::TOO_MANY_STREAMS);
    }
  }

  void MplexedConnection::onStream(NewStreamHandlerFunc cb) {
    new_stream_handler_ = std::move(cb);
  }

  outcome::result<peer::PeerId> MplexedConnection::localPeer() const {
    return connection_->localPeer();
  }

  outcome::result<peer::PeerId> MplexedConnection::remotePeer() const {
    return connection_->remotePeer();
  }

  outcome::result<crypto::PublicKey> MplexedConnection::remotePublicKey()
      const {
    return connection_->remotePublicKey();
  }

  bool MplexedConnection::isInitiator() const noexcept {
    return connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> MplexedConnection::localMultiaddr() {
    return connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> MplexedConnection::remoteMultiaddr() {
    return connection_->remoteMultiaddr();
  }

  outcome::result<void> MplexedConnection::close() {
    is_active_ = false;
    resetAllStreams();
    streams_.clear();
    return connection_->close();
  }

  bool MplexedConnection::isClosed() const {
    return !is_active_ || connection_->isClosed();
  }

  void MplexedConnection::read(gsl::span<uint8_t> out, size_t bytes,
                               ReadCallbackFunc cb) {
    connection_->read(out, bytes, std::move(cb));
  }

  void MplexedConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                   ReadCallbackFunc cb) {
    connection_->readSome(out, bytes, std::move(cb));
  }

  void MplexedConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                                WriteCallbackFunc cb) {
    connection_->write(in, bytes, std::move(cb));
  }

  void MplexedConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                    WriteCallbackFunc cb) {
    connection_->writeSome(in, bytes, std::move(cb));
  }

  void MplexedConnection::write(common::ByteArray bytes) {}

  void MplexedConnection::readNextFrame() {
    if (isClosed()) {
      return;
    }

    readFrame(connection_,
              [self{shared_from_this()}](auto &&frame_res) mutable {
                if (!frame_res) {
                  self->log_->error("cannot read frame from the connection: {}",
                                    frame_res.error().message());
                  return self->closeSession();
                }

                self->processFrame(std::move(frame_res.value()));
              });
  }

  void MplexedConnection::processFrame(MplexFrame frame) {
    using Flag = MplexFrame::Flag;

    // we are initiators of this connection, if the other side is a receiver of
    // this connection (o rly?)
    auto this_side_is_initiator = (frame.flag != Flag::NEW_STREAM)
        && (frame.flag == Flag::MESSAGE_RECEIVER
            || frame.flag == Flag::CLOSE_RECEIVER
            || frame.flag == Flag::RESET_RECEIVER);
    StreamId stream_id{frame.stream_number, this_side_is_initiator};

    switch (frame.flag) {
      case Flag::NEW_STREAM:
        processNewStreamFrame(frame, stream_id);
        break;
      case Flag::MESSAGE_RECEIVER:
      case Flag::MESSAGE_INITIATOR:
        processMessageFrame(frame, stream_id);
        break;
      case Flag::CLOSE_RECEIVER:
      case Flag::CLOSE_INITIATOR:
        processCloseFrame(frame, stream_id);
        break;
      case Flag::RESET_RECEIVER:
      case Flag::RESET_INITIATOR:
        processResetFrame(frame, stream_id);
        break;
      default:
        log_->critical("garbage in frame's flag");
        return closeSession();
    }

    readNextFrame();
  }

  void MplexedConnection::processNewStreamFrame(const MplexFrame &frame,
                                                StreamId stream_id) {
    if (streams_.size() >= config_.maximum_streams) {
      // reset the stream immediately
      return write(createFrameBytes(MplexFrame::Flag::RESET_RECEIVER,
                                    frame.stream_number));
    }

    // TODO: create a stream
  }

  /**
   * Find stream with such stream_id or reset it in case no such stream was
   * found
   */
#define FIND_STREAM_OR_RESET(stream_var_name, stream_id) \
  auto stream_opt = findStream(stream_id);               \
  if (!stream_opt) {                                     \
    return resetStream(stream_id);                       \
  }                                                      \
  auto stream_var_name = std::move(*stream_opt);

  void MplexedConnection::processMessageFrame(const MplexFrame &frame,
                                              StreamId stream_id) {
    FIND_STREAM_OR_RESET(stream, stream_id)

    // there is some data for this stream - commit it
    auto commit_res = stream->commitData(frame.data, frame.data.size());
    if (!commit_res) {
      return log_->error("failed to commit data for stream {}: {}", stream_id,
                         commit_res.error().message());
    }
  }

  void MplexedConnection::processCloseFrame(const MplexFrame &frame,
                                            StreamId stream_id) {
    FIND_STREAM_OR_RESET(stream, stream_id)

    // half-close the stream for reads; if it is already closed for writes as
    // well, remove it
    if (!stream->is_writable_) {
      return removeStream(stream_id);
    }
    stream->is_readable_ = true;
  }

  void MplexedConnection::processResetFrame(const MplexFrame &frame,
                                            StreamId stream_id) {
    if (auto stream_opt = findStream(stream_id); stream_opt) {
      (*stream_opt)->is_reset_ = true;
    }
    removeStream(stream_id);
  }

  boost::optional<std::shared_ptr<MplexStream>> MplexedConnection::findStream(
      const StreamId &id) const {
    if (auto stream_it = streams_.find(id); stream_it != streams_.end()) {
      return stream_it->second;
    }
    return {};
  }

  void MplexedConnection::removeStream(StreamId stream_id) {
    if (auto stream_opt = findStream(stream_id); stream_opt) {
      streams_.erase(stream_id);
      (*stream_opt)->is_writable_ = false;
      (*stream_opt)->is_readable_ = false;
    }
  }

  void MplexedConnection::resetStream(StreamId stream_id) {
    write(createFrameBytes(stream_id.initiator
                               ? MplexFrame::Flag::RESET_INITIATOR
                               : MplexFrame::Flag::RESET_RECEIVER,
                           stream_id.number));
  }

  void MplexedConnection::resetAllStreams() {
    for (const auto &id_stream_pair : streams_) {
      resetStream(id_stream_pair.first);
    }
  }

  void MplexedConnection::closeSession() {
    auto close_res = close();
    if (!close_res) {
      log_->error("cannot close the connection: {}",
                  close_res.error().message());
    }
  }

  void MplexedConnection::streamWrite(StreamId stream_id,
                                      gsl::span<const uint8_t> in, size_t bytes,
                                      bool some,
                                      basic::Writer::WriteCallbackFunc cb) {}

  void MplexedConnection::streamAckBytes(
      StreamId stream_id, uint32_t bytes,
      std::function<void(outcome::result<void>)> cb) {}

  void MplexedConnection::streamClose(
      StreamId stream_id, std::function<void(outcome::result<void>)> cb) {}

  void MplexedConnection::streamReset(
      StreamId stream_id, std::function<void(outcome::result<void>)> cb) {}
}  // namespace libp2p::connection
