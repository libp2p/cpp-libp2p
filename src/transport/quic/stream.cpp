/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lsquic.h>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>
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

  boost::asio::awaitable<outcome::result<size_t>> QuicStream::read(BytesOut out, size_t bytes) {
    ambigousSize(out, bytes);
    if (not stream_ctx_) {
      co_return QuicError::STREAM_CLOSED;
    }
    if (stream_ctx_->reading) {
      co_return QuicError::STREAM_READ_IN_PROGRESS;
    }
    auto n = lsquic_stream_read(stream_ctx_->ls_stream, out.data(), out.size());
    if (n == -1 && errno == EWOULDBLOCK) {
      bool done = false;
      outcome::result<size_t> r = QuicError::STREAM_CLOSED;
      stream_ctx_->reading.emplace(transport::lsquic::StreamCtx::Reading{
          out, [&](auto res) {
            r = res;
            done = true;
          }});
      lsquic_stream_wantread(stream_ctx_->ls_stream, 1);
      while (!done) {
        co_await boost::asio::post(boost::asio::use_awaitable);
      }
      co_return r;
    }
    if (n > 0) {
      co_return n;
    }
    co_return QuicError::STREAM_CLOSED;
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

  boost::asio::awaitable<outcome::result<size_t>> QuicStream::readSome(BytesOut out, size_t bytes) {
    ambigousSize(out, bytes);
    if (not stream_ctx_) {
      co_return QuicError::STREAM_CLOSED;
    }
    if (stream_ctx_->reading) {
      co_return QuicError::STREAM_READ_IN_PROGRESS;
    }
    auto n = lsquic_stream_read(stream_ctx_->ls_stream, out.data(), out.size());
    if (n == -1 && errno == EWOULDBLOCK) {
      bool done = false;
      outcome::result<size_t> r = QuicError::STREAM_CLOSED;
      stream_ctx_->reading.emplace(transport::lsquic::StreamCtx::Reading{
          out, [&](auto res) {
            r = res;
            done = true;
          }});
      lsquic_stream_wantread(stream_ctx_->ls_stream, 1);
      while (!done) {
        co_await boost::asio::post(boost::asio::use_awaitable);
      }
      co_return r;
    }
    if (n > 0) {
      co_return n;
    }
    co_return QuicError::STREAM_CLOSED;
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

  boost::asio::awaitable<std::error_code> QuicStream::writeSome(BytesIn in, size_t bytes) {
    ambigousSize(in, bytes);
    if (not stream_ctx_) {
      co_return QuicError::STREAM_CLOSED;
    }
    auto n = lsquic_stream_write(stream_ctx_->ls_stream, in.data(), in.size());
    if (n > 0 && lsquic_stream_flush(stream_ctx_->ls_stream) == 0) {
      stream_ctx_->engine->process();
      co_return std::error_code{};
    }
    stream_ctx_->engine->process();
    co_return QuicError::STREAM_CLOSED;
  }
}

