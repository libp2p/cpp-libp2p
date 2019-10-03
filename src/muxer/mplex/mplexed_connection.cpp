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
    data_subs_.clear();
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

    switch (frame.flag) {
      case Flag::NEW_STREAM:
        processNewStreamFrame(frame);
        break;
      case Flag::MESSAGE_RECEIVER:
      case Flag::MESSAGE_INITIATOR:
        processMessageFrame(frame);
        break;
      case Flag::CLOSE_RECEIVER:
      case Flag::CLOSE_INITIATOR:
        processCloseFrame(frame);
        break;
      case Flag::RESET_RECEIVER:
      case Flag::RESET_INITIATOR:
        processResetFrame(frame);
        break;
      default:
        log_->critical("garbage in frame's flag");
        return closeSession();
    }

    readNextFrame();
  }

  void MplexedConnection::processNewStreamFrame(const MplexFrame &frame) {
    if (streams_.size() >= config_.maximum_streams) {
      // reset the stream immediately
      return write(
          createFrameBytes(MplexFrame::Flag::RESET_RECEIVER, frame.stream_id));
    }

    // TODO: create a stream
  }

  void MplexedConnection::processMessageFrame(const MplexFrame &frame) {}

  void MplexedConnection::processCloseFrame(const MplexFrame &frame) {}

  void MplexedConnection::processResetFrame(const MplexFrame &frame) {}

  void MplexedConnection::resetAllStreams() const {}

  void MplexedConnection::closeSession() {
    auto close_res = close();
    if (!close_res) {
      log_->error("cannot close the connection: {}",
                  close_res.error().message());
    }
  }

  void MplexedConnection::streamOnWindowUpdate(StreamId stream_id,
                                               NotifyeeCallback cb) {}

  void MplexedConnection::streamOnAddData(StreamId stream_id,
                                          NotifyeeCallback cb) {}

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
