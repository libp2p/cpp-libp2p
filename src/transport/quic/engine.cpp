/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/asio/ssl/context.hpp>
#include <libp2p/common/asio_buffer.hpp>
#include <libp2p/common/asio_cb.hpp>
#include <libp2p/muxer/muxed_connection_config.hpp>
#include <libp2p/security/tls/tls_details.hpp>
#include <libp2p/security/tls/tls_errors.hpp>
#include <libp2p/transport/quic/connection.hpp>
#include <libp2p/transport/quic/engine.hpp>
#include <libp2p/transport/quic/error.hpp>
#include <libp2p/transport/quic/init.hpp>
#include <libp2p/transport/quic/stream.hpp>
#include <libp2p/transport/tcp/tcp_util.hpp>
#include <qtils/option_take.hpp>

namespace libp2p::transport::lsquic {
  Engine::Engine(std::shared_ptr<boost::asio::io_context> io_context,
                 std::shared_ptr<boost::asio::ssl::context> ssl_context,
                 const muxer::MuxedConnectionConfig &mux_config,
                 PeerId local_peer,
                 std::shared_ptr<crypto::marshaller::KeyMarshaller> key_codec,
                 boost::asio::ip::udp::socket &&socket,
                 bool client)
      : io_context_{std::move(io_context)},
        ssl_context_{std::move(ssl_context)},
        local_peer_{std::move(local_peer)},
        key_codec_{std::move(key_codec)},
        socket_{std::move(socket)},
        timer_{*io_context_},
        socket_local_{socket_.local_endpoint()},
        local_{detail::makeQuicAddr(socket_local_).value()} {
    socket_.non_blocking(true);

    lsquicInit();

    auto flags = 0;
    if (not client) {
      flags |= LSENG_SERVER;
    }

    lsquic_engine_settings settings{};
    lsquic_engine_init_settings(&settings, flags);
    settings.es_init_max_stream_data_bidi_remote =
        mux_config.maximum_window_size;
    settings.es_init_max_stream_data_bidi_local =
        mux_config.maximum_window_size;
    settings.es_init_max_streams_bidi = mux_config.maximum_streams;
    settings.es_idle_timeout = std::chrono::duration_cast<std::chrono::seconds>(
                                   mux_config.no_streams_interval)
                                   .count();

    static lsquic_stream_if stream_if{};
    stream_if.on_new_conn = +[](void *void_self, lsquic_conn_t *conn) {
      auto self = static_cast<Engine *>(void_self);
      auto op = qtils::optionTake(self->connecting_);
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      auto conn_ctx = new ConnCtx{self, conn, std::move(op)};
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto _conn_ctx = reinterpret_cast<lsquic_conn_ctx_t *>(conn_ctx);
      lsquic_conn_set_ctx(conn, _conn_ctx);
      if (not op) {
        stream_if.on_hsk_done(conn, LSQ_HSK_OK);
      }
      return _conn_ctx;
    };
    stream_if.on_conn_closed = +[](lsquic_conn_t *conn) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto conn_ctx = reinterpret_cast<ConnCtx *>(lsquic_conn_get_ctx(conn));
      if (auto op = qtils::optionTake(conn_ctx->connecting)) {
        op->cb(QuicError::CONN_CLOSED);
      }
      if (auto conn = conn_ctx->conn.lock()) {
        conn->onClose();
      }
      lsquic_conn_set_ctx(conn, nullptr);
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      delete conn_ctx;
    };
    stream_if.on_hsk_done = +[](lsquic_conn_t *conn, lsquic_hsk_status status) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto conn_ctx = reinterpret_cast<ConnCtx *>(lsquic_conn_get_ctx(conn));
      auto self = conn_ctx->engine;
      auto ok = status == LSQ_HSK_OK or status == LSQ_HSK_RESUMED_OK;
      auto op = qtils::optionTake(conn_ctx->connecting);
      auto res = [&]() -> outcome::result<std::shared_ptr<QuicConnection>> {
        if (not ok) {
          return QuicError::HANDSHAKE_FAILED;
        }
        auto cert = SSL_get_peer_certificate(lsquic_conn_ssl(conn));
        OUTCOME_TRY(info,
                    security::tls_details::verifyPeerAndExtractIdentity(
                        cert, *self->key_codec_));
        if (op and info.peer_id != op->peer) {
          return security::TlsError::TLS_UNEXPECTED_PEER_ID;
        }
        auto conn = std::make_shared<QuicConnection>(
            self->io_context_,
            conn_ctx,
            op.has_value(),
            self->local_,
            detail::makeQuicAddr(op->remote).value(),
            self->local_peer_,
            info.peer_id,
            info.public_key);
        conn_ctx->conn = conn;
        return conn;
      }();
      if (not res) {
        lsquic_conn_close(conn);
      }
      if (op) {
        op->cb(res);
      } else if (res) {
        self->on_accept_(res.value());
      }
    };
    stream_if.on_new_stream = +[](void *void_self, lsquic_stream_t *stream) {
      auto self = static_cast<Engine *>(void_self);
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      auto conn_ctx = reinterpret_cast<ConnCtx *>(
          lsquic_conn_get_ctx(lsquic_stream_conn(stream)));
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      auto stream_ctx = new StreamCtx{self, stream};
      if (auto conn = conn_ctx->conn.lock()) {
        auto stream = std::make_shared<QuicStream>(
            conn, stream_ctx, conn_ctx->new_stream.has_value());
        stream_ctx->stream = stream;
        if (conn_ctx->new_stream) {
          *conn_ctx->new_stream = stream;
        } else {
          conn->onStream()(stream);
        }
      } else {
        lsquic_stream_close(stream);
      }
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      return reinterpret_cast<lsquic_stream_ctx_t *>(stream_ctx);
    };
    stream_if.on_close =
        +[](lsquic_stream_t *stream, lsquic_stream_ctx_t *_stream_ctx) {
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          auto stream_ctx = reinterpret_cast<StreamCtx *>(_stream_ctx);
          if (auto op = qtils::optionTake(stream_ctx->reading)) {
            op->cb(QuicError::STREAM_CLOSED);
          }
          if (auto stream = stream_ctx->stream.lock()) {
            stream->onClose();
          }
          // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
          delete stream_ctx;
        };
    stream_if.on_read =
        +[](lsquic_stream_t *stream, lsquic_stream_ctx_t *_stream_ctx) {
          lsquic_stream_wantread(stream, 0);
          // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
          auto stream_ctx = reinterpret_cast<StreamCtx *>(_stream_ctx);
          auto op = qtils::optionTake(stream_ctx->reading).value();
          auto n = lsquic_stream_read(stream, op.out.data(), op.out.size());
          outcome::result<size_t> r = QuicError::STREAM_CLOSED;
          if (n > 0) {
            r = n;
          }
          stream_ctx->engine->io_context_->post(
              [cb{std::move(op.cb)}, r] { cb(r); });
        };

    lsquic_engine_api api{};
    api.ea_settings = &settings;

    api.ea_stream_if = &stream_if;
    api.ea_stream_if_ctx = this;
    api.ea_packets_out = +[](void *void_self,
                             const lsquic_out_spec *out_spec,
                             unsigned n_packets_out) {
      auto self = static_cast<Engine *>(void_self);
      // https://github.com/cbodley/nexus/blob/d1d8486f713fd089917331239d755932c7c8ed8e/src/socket.cc#L218
      int r = 0;
      for (auto &spec : std::span{out_spec, n_packets_out}) {
        msghdr msg{};
        msg.msg_iov = spec.iov;
        msg.msg_iovlen = spec.iovlen;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        msg.msg_name = const_cast<sockaddr *>(spec.dest_sa);
        msg.msg_namelen = spec.dest_sa->sa_family == AF_INET
                            ? sizeof(sockaddr_in)
                            : sizeof(sockaddr_in6);
        auto n = sendmsg(self->socket_.native_handle(), &msg, 0);
        if (n == -1) {
          if (errno == EAGAIN or errno == EWOULDBLOCK) {
            auto cb = [weak_self{self->weak_from_this()}](
                          boost::system::error_code ec) {
              auto self = weak_self.lock();
              if (not self) {
                return;
              }
              if (ec) {
                return;
              }
              lsquic_engine_send_unsent_packets(self->engine_);
            };
            self->socket_.async_wait(boost::asio::socket_base::wait_write,
                                     std::move(cb));
          }
          break;
        }
        ++r;
      }
      return r;
    };
    api.ea_packets_out_ctx = this;
    api.ea_get_ssl_ctx = +[](void *void_self, const sockaddr *) {
      auto self = static_cast<Engine *>(void_self);
      return self->ssl_context_->native_handle();
    };

    engine_ = lsquic_engine_new(flags, &api);
    if (not engine_) {
      throw std::logic_error{"lsquic_engine_new"};
    }
  }

  Engine::~Engine() {
    lsquic_engine_destroy(engine_);
  }

  void Engine::start() {
    if (started_) {
      return;
    }
    started_ = true;
    readLoop();
  }

  void Engine::connect(const boost::asio::ip::udp::endpoint &remote,
                       const PeerId &peer,
                       OnConnect cb) {
    if (connecting_) {
      throw std::logic_error{"Engine::connect invalid state"};
    }
    connecting_ = Connecting{remote, peer, std::move(cb)};
    start();
    lsquic_engine_connect(engine_,
                          N_LSQVER,
                          socket_local_.data(),
                          remote.data(),
                          this,
                          nullptr,
                          nullptr,
                          0,
                          nullptr,
                          0,
                          nullptr,
                          0);
    if (auto op = qtils::optionTake(connecting_)) {
      op->cb(QuicError::CANT_CREATE_CONNECTION);
    }
    process();
  }

  outcome::result<std::shared_ptr<QuicStream>> Engine::newStream(
      ConnCtx *conn_ctx) {
    if (conn_ctx->new_stream) {
      throw std::logic_error{"Engine::newStream invalid state"};
    }
    if (lsquic_conn_n_pending_streams(conn_ctx->ls_conn) != 0) {
      return QuicError::TOO_MANY_STREAMS;
    }
    conn_ctx->new_stream.emplace();
    lsquic_conn_make_stream(conn_ctx->ls_conn);
    auto stream = qtils::optionTake(conn_ctx->new_stream).value();
    if (not stream) {
      return QuicError::CANT_OPEN_STREAM;
    }
    return stream;
  }

  void Engine::process() {
    lsquic_engine_process_conns(engine_);
    int us = 0;
    if (not lsquic_engine_earliest_adv_tick(engine_, &us)) {
      return;
    }
    timer_.expires_after(std::chrono::microseconds{us});
    auto cb = [weak_self{weak_from_this()}](boost::system::error_code ec) {
      auto self = weak_self.lock();
      if (not self) {
        return;
      }
      if (ec) {
        return;
      }
      self->process();
    };
    timer_.async_wait(std::move(cb));
  }

  void Engine::readLoop() {
    // https://github.com/cbodley/nexus/blob/d1d8486f713fd089917331239d755932c7c8ed8e/src/socket.cc#L293
    while (true) {
      socklen_t len = socket_local_.size();
      auto n = recvfrom(socket_.native_handle(),
                        reading_.buf.data(),
                        reading_.buf.size(),
                        0,
                        reading_.remote.data(),
                        &len);
      if (n == -1) {
        if (errno == EAGAIN or errno == EWOULDBLOCK) {
          auto cb =
              [weak_self{weak_from_this()}](boost::system::error_code ec) {
                auto self = weak_self.lock();
                if (not self) {
                  return;
                }
                if (ec) {
                  return;
                }
                self->readLoop();
              };
          socket_.async_wait(boost::asio::socket_base::wait_read,
                             std::move(cb));
        }
        return;
      }
      lsquic_engine_packet_in(engine_,
                              reading_.buf.data(),
                              n,
                              socket_local_.data(),
                              reading_.remote.data(),
                              this,
                              0);
      process();
    }
  }
}  // namespace libp2p::transport::lsquic
