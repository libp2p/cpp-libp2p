/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/noise_connection.hpp>

#include <libp2p/crypto/x25519_provider/x25519_provider_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

#ifndef UNIQUE_NAME
#define UNIQUE_NAME(base) base##__LINE__
#endif  // UNIQUE_NAME

#define OUTCOME_CB_I(var, res)                \
  auto && (var) = (res);                      \
  if ((var).has_error()) {                    \
    self->eraseWriteBuffer(ctx.write_buffer); \
    return cb((var).error());                 \
  }

#define OUTCOME_CB_NAME_I(var, val, res) \
  OUTCOME_CB_I(var, res)                 \
  auto && (val) = (var).value();

#define OUTCOME_CB(name, res) OUTCOME_CB_NAME_I(UNIQUE_NAME(name), name, res)

namespace libp2p::connection {
  NoiseConnection::NoiseConnection(
      std::shared_ptr<LayerConnection> original_connection,
      crypto::PublicKey localPubkey, crypto::PublicKey remotePubkey,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      std::shared_ptr<security::noise::CipherState> encoder,
      std::shared_ptr<security::noise::CipherState> decoder)
      : connection_{std::move(original_connection)},
        local_{std::move(localPubkey)},
        remote_{std::move(remotePubkey)},
        key_marshaller_{std::move(key_marshaller)},
        encoder_cs_{std::move(encoder)},
        decoder_cs_{std::move(decoder)},
        frame_buffer_{
            std::make_shared<common::ByteArray>(security::noise::kMaxMsgLen)},
        framer_{std::make_shared<security::noise::InsecureReadWriter>(
            connection_, frame_buffer_)} {
    BOOST_ASSERT(connection_);
    BOOST_ASSERT(key_marshaller_);
    BOOST_ASSERT(encoder_cs_);
    BOOST_ASSERT(decoder_cs_);
    BOOST_ASSERT(frame_buffer_);
    BOOST_ASSERT(framer_);
    frame_buffer_->resize(0);
  }

  bool NoiseConnection::isClosed() const {
    return connection_->isClosed();
  }

  outcome::result<void> NoiseConnection::close() {
    return connection_->close();
  }

  void NoiseConnection::read(gsl::span<uint8_t> out, size_t bytes,
                             libp2p::basic::Reader::ReadCallbackFunc cb) {
    OperationContext context{.bytes_served = 0,
                             .total_bytes = bytes,
                             .write_buffer = write_buffers_.end()};
    read(out, bytes, context, std::move(cb));
  }

  void NoiseConnection::read(gsl::span<uint8_t> out, size_t bytes,
                             OperationContext ctx, ReadCallbackFunc cb) {
    size_t out_size{out.empty() ? 0u : static_cast<size_t>(out.size())};
    BOOST_ASSERT(out_size >= bytes);
    if (0 == bytes) {
      BOOST_ASSERT(ctx.bytes_served == ctx.total_bytes);
      return cb(ctx.bytes_served);
    }
    readSome(out, bytes,
             [self{shared_from_this()}, out, bytes, cb{std::move(cb)},
              ctx](auto _n) mutable {
               OUTCOME_CB(n, _n);
               ctx.bytes_served += n;
               self->read(out.subspan(n), bytes - n, ctx, std::move(cb));
             });
  }

  void NoiseConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                 libp2p::basic::Reader::ReadCallbackFunc cb) {
    OperationContext context{.bytes_served = 0,
                             .total_bytes = bytes,
                             .write_buffer = write_buffers_.end()};
    readSome(out, bytes, context, std::move(cb));
  }

  void NoiseConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
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
      OUTCOME_CB(decrypted, self->decoder_cs_->decrypt({}, *data, {}));
      self->frame_buffer_->assign(decrypted.begin(), decrypted.end());
      self->readSome(out, bytes, ctx, std::move(cb));
    });
  }

  void NoiseConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                              libp2p::basic::Writer::WriteCallbackFunc cb) {
    OperationContext context{.bytes_served = 0,
                             .total_bytes = bytes,
                             .write_buffer = write_buffers_.end()};
    write(in, bytes, context, std::move(cb));
  }

  void NoiseConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                              NoiseConnection::OperationContext ctx,
                              basic::Writer::WriteCallbackFunc cb) {
    auto *self{this};  // for OUTCOME_CB
    if (0 == bytes) {
      BOOST_ASSERT(ctx.bytes_served >= ctx.total_bytes);
      eraseWriteBuffer(ctx.write_buffer);
      return cb(ctx.total_bytes);
    }
    auto n{std::min(bytes, security::noise::kMaxPlainText)};
    OUTCOME_CB(encrypted, encoder_cs_->encrypt({}, in.subspan(0, n), {}));
    if (write_buffers_.end() == ctx.write_buffer) {
      constexpr auto dummy_size = 1;
      constexpr auto dummy_value = 0x0;
      ctx.write_buffer =
          write_buffers_.emplace(write_buffers_.end(), dummy_size, dummy_value);
    }
    ctx.write_buffer->swap(encrypted);
    framer_->write(
        *ctx.write_buffer,
        [self{shared_from_this()}, in{in.subspan(static_cast<int64_t>(n))},
         bytes{bytes - n}, cb{std::move(cb)}, ctx](auto _n) mutable {
          OUTCOME_CB(n, _n);
          ctx.bytes_served += n;
          self->write(in, bytes, ctx, std::move(cb));
        });
  }

  void NoiseConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                  libp2p::basic::Writer::WriteCallbackFunc cb) {
    write(in, bytes, std::move(cb));
  }

  void NoiseConnection::deferReadCallback(outcome::result<size_t> res,
                                          ReadCallbackFunc cb) {
    connection_->deferReadCallback(res, std::move(cb));
  }

  void NoiseConnection::deferWriteCallback(std::error_code ec,
                                           WriteCallbackFunc cb) {
    connection_->deferWriteCallback(ec, std::move(cb));
  }

  bool NoiseConnection::isInitiator() const noexcept {
    return connection_->isInitiator();
  }

  outcome::result<libp2p::multi::Multiaddress>
  NoiseConnection::localMultiaddr() {
    return connection_->localMultiaddr();
  }

  outcome::result<libp2p::multi::Multiaddress>
  NoiseConnection::remoteMultiaddr() {
    return connection_->remoteMultiaddr();
  }

  outcome::result<libp2p::peer::PeerId> NoiseConnection::localPeer() const {
    OUTCOME_TRY(proto_local_key, key_marshaller_->marshal(local_));
    return peer::PeerId::fromPublicKey(proto_local_key);
  }

  outcome::result<libp2p::peer::PeerId> NoiseConnection::remotePeer() const {
    OUTCOME_TRY(proto_remote_key, key_marshaller_->marshal(remote_));
    return peer::PeerId::fromPublicKey(proto_remote_key);
  }

  outcome::result<libp2p::crypto::PublicKey> NoiseConnection::remotePublicKey()
      const {
    return remote_;
  }

  void NoiseConnection::eraseWriteBuffer(BufferList::iterator &iterator) {
    if (write_buffers_.end() == iterator) {
      return;
    }
    write_buffers_.erase(iterator);
    iterator = write_buffers_.end();
  }
}  // namespace libp2p::connection
