/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/asio/use_awaitable.hpp>
#include <libp2p/security/noise/noise_connection.hpp>

#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/common/ambigous_size.hpp>
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
      crypto::PublicKey localPubkey,
      crypto::PublicKey remotePubkey,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      std::shared_ptr<security::noise::CipherState> encoder,
      std::shared_ptr<security::noise::CipherState> decoder)
      : connection_{std::move(original_connection)},
        local_{std::move(localPubkey)},
        remote_{std::move(remotePubkey)},
        key_marshaller_{std::move(key_marshaller)},
        encoder_cs_{std::move(encoder)},
        decoder_cs_{std::move(decoder)},
        frame_buffer_{std::make_shared<Bytes>(security::noise::kMaxMsgLen)},
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

  void NoiseConnection::read(BytesOut out,
                             size_t bytes,
                             libp2p::basic::Reader::ReadCallbackFunc cb) {
    ambigousSize(out, bytes);
    readReturnSize(shared_from_this(), out, std::move(cb));
  }

  void NoiseConnection::readSome(BytesOut out,
                                 size_t bytes,
                                 libp2p::basic::Reader::ReadCallbackFunc cb) {
    OperationContext context{.bytes_served = 0,
                             .total_bytes = bytes,
                             .write_buffer = write_buffers_.end()};
    readSome(out, bytes, context, std::move(cb));
  }

  void NoiseConnection::readSome(BytesOut out,
                                 size_t bytes,
                                 OperationContext ctx,
                                 ReadCallbackFunc cb) {
    if (not frame_buffer_->empty()) {
      auto n{std::min(bytes, frame_buffer_->size())};
      auto begin{frame_buffer_->begin()};
      auto end{begin + static_cast<int64_t>(n)};
      std::copy(begin, end, out.begin());
      frame_buffer_->erase(begin, end);
      return cb(n);
    }
    framer_->read(
        [self{shared_from_this()}, out, bytes, cb{std::move(cb)}, ctx](
            auto _data) mutable {
          OUTCOME_CB(data, _data);
          OUTCOME_CB(decrypted, self->decoder_cs_->decrypt({}, *data, {}));
          self->frame_buffer_->assign(decrypted.begin(), decrypted.end());
          self->readSome(out, bytes, ctx, std::move(cb));
        });
  }

  void NoiseConnection::write(BytesIn in,
                              size_t bytes,
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
    framer_->write(*ctx.write_buffer,
                   [self{shared_from_this()},
                    in{in.subspan(static_cast<int64_t>(n))},
                    bytes{bytes - n},
                    cb{std::move(cb)},
                    ctx](auto _n) mutable {
                     OUTCOME_CB(n, _n);
                     ctx.bytes_served += n;
                     self->write(in, bytes, ctx, std::move(cb));
                   });
  }

  void NoiseConnection::writeSome(BytesIn in,
                                  size_t bytes,
                                  libp2p::basic::Writer::WriteCallbackFunc cb) {
    OperationContext context{
        .bytes_served = 0,
        .total_bytes = bytes,
        .write_buffer = write_buffers_.end(),
    };
    write(in, bytes, context, std::move(cb));
  }

  void NoiseConnection::deferReadCallback(outcome::result<size_t> res,
                                          ReadCallbackFunc cb) {
    connection_->deferReadCallback(res, std::move(cb));
  }

  void NoiseConnection::deferWriteCallback(std::error_code ec,
                                           WriteCallbackFunc cb) {
    connection_->deferWriteCallback(ec, std::move(cb));
  }

  bool NoiseConnection::isInitiator() const {
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

  boost::asio::awaitable<outcome::result<size_t>> NoiseConnection::read(BytesOut out,
                                                                        size_t bytes) {
    ambigousSize(out, bytes);

    size_t total_bytes_read = 0;
    while (total_bytes_read < bytes) {
      auto remaining = bytes - total_bytes_read;
      auto result = co_await readSome(out.subspan(total_bytes_read), remaining);

      if (!result) {
        co_return result.error();
      }

      size_t bytes_read = result.value();
      if (bytes_read == 0) {
        break;  // Connection closed by peer
      }

      total_bytes_read += bytes_read;
    }

    co_return total_bytes_read;
  }

  boost::asio::awaitable<outcome::result<size_t>> NoiseConnection::readSome(
      BytesOut out, size_t bytes) {
    ambigousSize(out, bytes);

    // If there's data in the frame buffer, use it directly
    if (!frame_buffer_->empty()) {
      auto n = std::min(bytes, frame_buffer_->size());
      auto begin = frame_buffer_->begin();
      auto end = begin + static_cast<int64_t>(n);
      std::copy(begin, end, out.begin());
      frame_buffer_->erase(begin, end);
      co_return n;
    }

    // No data in buffer, need to read a new frame using coroutine method
    auto frame_result = co_await framer_->read();
    if (!frame_result) {
      co_return frame_result.error();
    }

    // Decrypt the received data
    auto decrypt_result = decoder_cs_->decrypt({}, *frame_result.value(), {});
    if (!decrypt_result) {
      co_return decrypt_result.error();
    }

    // Store decrypted data in frame buffer
    frame_buffer_->assign(decrypt_result.value().begin(), decrypt_result.value().end());

    // Now read from the frame buffer
    auto n = std::min(bytes, frame_buffer_->size());
    auto begin = frame_buffer_->begin();
    auto end = begin + static_cast<int64_t>(n);
    std::copy(begin, end, out.begin());
    frame_buffer_->erase(begin, end);

    co_return n;
  }

  boost::asio::awaitable<std::error_code> NoiseConnection::writeSome(
      BytesIn in, size_t bytes) {
    ambigousSize(in, bytes);

    if (0 == bytes) {
      co_return std::error_code{};
    }

    // Process only up to kMaxPlainText bytes at a time
    auto n = std::min(bytes, security::noise::kMaxPlainText);

    // Encrypt the data
    auto encrypt_result = encoder_cs_->encrypt({}, in.subspan(0, n), {});
    if (!encrypt_result) {
      co_return encrypt_result.error();
    }

    // Store the encrypted data in a buffer
    Bytes encrypted = std::move(encrypt_result.value());

    // Write the encrypted data using coroutine method
    auto write_result = co_await framer_->write(encrypted);
    if (!write_result) {
      co_return write_result.error();
    }

    // If there's more data to write, recursively call writeSome
    if (n < bytes) {
      auto remaining_result = co_await writeSome(in.subspan(n), bytes - n);
      if (remaining_result) {
        co_return std::error_code{};
      }
      co_return remaining_result;
    }

    co_return std::error_code{};
  }

}  // namespace libp2p::connection
