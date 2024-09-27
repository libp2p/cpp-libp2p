/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lsquic.h>
#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/common/ambigous_size.hpp>
#include <libp2p/transport/quic/connection.hpp>
#include <libp2p/transport/quic/engine.hpp>
#include <libp2p/transport/quic/error.hpp>
#include <libp2p/transport/quic/stream.hpp>

namespace libp2p::connection {
  using transport::lsquic::StreamCtx;

  QuicStream::QuicStream(std::shared_ptr<transport::QuicConnection> conn,
                         StreamCtx *stream_ctx,
                         bool initiator)
      : conn_{std::move(conn)},
        stream_ctx_{stream_ctx},
        initiator_{initiator} {}

  QuicStream::~QuicStream() {
    reset();
  }

  void QuicStream::read(BytesOut out,
                        size_t bytes,
                        basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    readReturnSize(shared_from_this(), out, std::move(cb));
  }

  void QuicStream::readSome(BytesOut out,
                            size_t bytes,
                            basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    outcome::result<size_t> r = QuicError::STREAM_CLOSED;
    if (not stream_ctx_) {
      return cb(r);
    }
    if (stream_ctx_->reading) {
      throw std::logic_error{"QuicStream::readSome already in progress"};
    }
    auto n = lsquic_stream_read(stream_ctx_->ls_stream, out.data(), out.size());
    if (n == -1 and errno == EWOULDBLOCK) {
      stream_ctx_->reading.emplace(StreamCtx::Reading{out, std::move(cb)});
      lsquic_stream_wantread(stream_ctx_->ls_stream, 1);
      return;
    }
    if (n > 0) {
      r = n;
    }
    deferReadCallback(r, std::move(cb));
  }

  void QuicStream::deferReadCallback(outcome::result<size_t> res,
                                     basic::Reader::ReadCallbackFunc cb) {
    conn_->deferReadCallback(res, std::move(cb));
  }

  void QuicStream::writeSome(BytesIn in,
                             size_t bytes,
                             basic::Writer::WriteCallbackFunc cb) {
    ambigousSize(in, bytes);
    outcome::result<size_t> r = QuicError::STREAM_CLOSED;
    if (not stream_ctx_) {
      return cb(r);
    }
    auto n = lsquic_stream_write(stream_ctx_->ls_stream, in.data(), in.size());
    if (n > 0 and lsquic_stream_flush(stream_ctx_->ls_stream) == 0) {
      r = n;
    }
    stream_ctx_->engine->process();
    deferReadCallback(r, std::move(cb));
  }

  void QuicStream::deferWriteCallback(std::error_code ec,
                                      WriteCallbackFunc cb) {
    conn_->deferWriteCallback(ec, std::move(cb));
  }

  bool QuicStream::isClosedForRead() const {
    return false;  // deprecated
  }

  bool QuicStream::isClosedForWrite() const {
    return false;  // deprecated
  }

  bool QuicStream::isClosed() const {
    return false;  // deprecated
  }

  void QuicStream::close(Stream::VoidResultHandlerFunc cb) {
    if (not stream_ctx_) {
      return cb(outcome::success());
    }
    lsquic_stream_shutdown(stream_ctx_->ls_stream, 1);
    cb(outcome::success());
  }

  void QuicStream::reset() {
    if (not stream_ctx_) {
      return;
    }
    lsquic_stream_close(stream_ctx_->ls_stream);
  }

  void QuicStream::adjustWindowSize(uint32_t new_size,
                                    Stream::VoidResultHandlerFunc cb) {}

  outcome::result<bool> QuicStream::isInitiator() const {
    return initiator_;
  }

  outcome::result<PeerId> QuicStream::remotePeerId() const {
    return conn_->remotePeer();
  }

  outcome::result<Multiaddress> QuicStream::localMultiaddr() const {
    return conn_->localMultiaddr();
  }

  outcome::result<Multiaddress> QuicStream::remoteMultiaddr() const {
    return conn_->remoteMultiaddr();
  }

  void QuicStream::onClose() {
    stream_ctx_ = nullptr;
  }
}  // namespace libp2p::connection
