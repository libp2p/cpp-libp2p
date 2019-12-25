/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/secio/secio_connection.hpp>

#include <algorithm>

#include <arpa/inet.h>
#include <libp2p/common/byteutil.hpp>
#include <libp2p/crypto/aes_provider.hpp>
#include <libp2p/crypto/error.hpp>
#include <libp2p/crypto/hmac_provider.hpp>
#include <libp2p/outcome/outcome.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, SecioConnection::Error, e) {
  using E = libp2p::connection::SecioConnection::Error;
  switch (e) {
    case E::CONN_NOT_INITIALIZED:
      return "Connection is not initialized";
    case E::CONN_ALREADY_INITIALIZED:
      return "Connection was already initialized";
    case E::INITALIZATION_FAILED:
      return "Connection initialization failed";
    case E::UNSUPPORTED_CIPHER:
      return "Unsupported cipher algorithm is used";
    case E::UNSUPPORTED_HASH:
      return "Unsupported hash algorithm is used";
    case E::INVALID_MAC:
      return "Received message has an invalid signature";
    case E::TOO_SHORT_BUFFER:
      return "Provided buffer is too short";
    case E::NOTHING_TO_READ:
      return "Zero bytes available to read";
    case E::STREAM_IS_BROKEN:
      return "Stream cannot be read. Frames are corrupted";
    case E::OVERSIZED_FRAME:
      return "Frame exceeds the maximum allowed size";
    default:
      return "Unknown error";
  }
}

#define IO_OUTCOME_TRY(name, res, cb) \
  if ((res).has_error()) {            \
    cb((res).error());                \
    return;                           \
  }                                   \
  auto(name) = (res).value();

namespace {
  template <typename AesSecretType>
  libp2p::outcome::result<AesSecretType> initAesSecret(
      const libp2p::common::ByteArray &key,
      const libp2p::common::ByteArray &iv) {
    AesSecretType secret{};
    if (key.size() != secret.key.size()) {
      return libp2p::crypto::OpenSslError::WRONG_KEY_SIZE;
    }
    if (iv.size() != secret.iv.size()) {
      return libp2p::crypto::OpenSslError::WRONG_IV_SIZE;
    }
    std::copy(key.begin(), key.end(), secret.key.begin());
    std::copy(iv.begin(), iv.end(), secret.iv.begin());
    return secret;
  }
}  // namespace

namespace libp2p::connection {

  SecioConnection::SecioConnection(
      std::shared_ptr<RawConnection> raw_connection,
      std::shared_ptr<crypto::hmac::HmacProvider> hmac_provider,
      std::shared_ptr<crypto::aes::AesProvider> aes_provider,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      crypto::PublicKey local_pubkey, crypto::PublicKey remote_pubkey,
      crypto::common::HashType hash_type,
      crypto::common::CipherType cipher_type,
      crypto::StretchedKey local_stretched_key,
      crypto::StretchedKey remote_stretched_key)
      : raw_connection_{std::move(raw_connection)},
        hmac_provider_{std::move(hmac_provider)},
        aes_provider_{std::move(aes_provider)},
        key_marshaller_{std::move(key_marshaller)},
        local_{std::move(local_pubkey)},
        remote_{std::move(remote_pubkey)},
        hash_type_{hash_type},
        cipher_type_{cipher_type},
        local_stretched_key_{std::move(local_stretched_key)},
        remote_stretched_key_{std::move(remote_stretched_key)},
        aes128_secrets_{boost::none},
        aes256_secrets_{boost::none},
        reader_is_ready_{true},
        read_completed_{true} {
    BOOST_ASSERT(raw_connection_);
    BOOST_ASSERT(hmac_provider_);
    BOOST_ASSERT(aes_provider_);
    BOOST_ASSERT(key_marshaller_);
    BOOST_ASSERT(reader_is_ready_);
    BOOST_ASSERT(read_completed_);
  }

  outcome::result<void> SecioConnection::init() {
    if (isInitialised()) {
      return Error::CONN_ALREADY_INITIALIZED;
    }

    using CT = crypto::common::CipherType;
    if (cipher_type_ == CT::AES128) {
      OUTCOME_TRY(
          local_128,
          initAesSecret<crypto::common::Aes128Secret>(
              local_stretched_key_.cipher_key, local_stretched_key_.iv));
      OUTCOME_TRY(
          remote_128,
          initAesSecret<crypto::common::Aes128Secret>(
              remote_stretched_key_.cipher_key, remote_stretched_key_.iv));
      aes128_secrets_ = AesSecrets<crypto::common::Aes128Secret>{
          .local = local_128, .remote = remote_128};
    } else if (cipher_type_ == CT::AES256) {
      OUTCOME_TRY(
          local_256,
          initAesSecret<crypto::common::Aes256Secret>(
              local_stretched_key_.cipher_key, local_stretched_key_.iv));
      OUTCOME_TRY(
          remote_256,
          initAesSecret<crypto::common::Aes256Secret>(
              remote_stretched_key_.cipher_key, remote_stretched_key_.iv));
      aes256_secrets_ = AesSecrets<crypto::common::Aes256Secret>{
          .local = local_256, .remote = remote_256};
    } else {
      return Error::UNSUPPORTED_CIPHER;
    }

    return outcome::success();
  }

  bool SecioConnection::isInitialised() const {
    return aes128_secrets_.has_value() != aes256_secrets_.has_value();
  }

  outcome::result<peer::PeerId> SecioConnection::localPeer() const {
    OUTCOME_TRY(proto_local_key, key_marshaller_->marshal(local_));
    return peer::PeerId::fromPublicKey(proto_local_key);
  }

  outcome::result<peer::PeerId> SecioConnection::remotePeer() const {
    OUTCOME_TRY(proto_remote_key, key_marshaller_->marshal(remote_));
    return peer::PeerId::fromPublicKey(proto_remote_key);
  }

  outcome::result<crypto::PublicKey> SecioConnection::remotePublicKey() const {
    return remote_;
  }

  bool SecioConnection::isInitiator() const noexcept {
    return raw_connection_->isInitiator();
  }

  outcome::result<multi::Multiaddress> SecioConnection::localMultiaddr() {
    return raw_connection_->localMultiaddr();
  }

  outcome::result<multi::Multiaddress> SecioConnection::remoteMultiaddr() {
    return raw_connection_->remoteMultiaddr();
  }

  void SecioConnection::read(gsl::span<uint8_t> out, size_t bytes,
                             basic::Reader::ReadCallbackFunc cb) {
    // the line below is due to gsl::span.size() has signed return type
    size_t out_size{out.empty() ? 0u : static_cast<size_t>(out.size())};
    if (out_size < bytes) {
      cb(Error::TOO_SHORT_BUFFER);
    }
    readSome(out, bytes, std::move(cb));
  }

  void SecioConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                 basic::Reader::ReadCallbackFunc cb) {
    if (!isInitialised()) {
      cb(Error::CONN_NOT_INITIALIZED);
    }

    {
      std::unique_lock<std::mutex> lock(read_mutex_);
      reader_is_ready_cv_.wait(lock,
                               [this] { return reader_is_ready_.load(); });
      // here lock becomes acquired
      reader_is_ready_ = false;
    }
    size_t out_size{out.empty() ? 0 : static_cast<size_t>(out.size())};
    size_t to_read{out_size < bytes ? out_size : bytes};

    while (user_data_buffer_.size() < to_read) {
      IO_OUTCOME_TRY(bytes_read, readMessageSynced(), cb)
      if (0 == bytes_read) {
        cb(Error::NOTHING_TO_READ);
        reader_is_ready_ = true;
        reader_is_ready_cv_.notify_all();
        return;
      }
    }

    auto to{out.begin()};
    for (size_t read = 0; read < to_read; ++read) {
      *to = user_data_buffer_.front();
      user_data_buffer_.pop();
      ++to;
    }
    reader_is_ready_ = true;
    reader_is_ready_cv_.notify_all();
    cb(to_read);
  }

  outcome::result<size_t> SecioConnection::readMessageSynced() {
    outcome::result<size_t> result{Error::STREAM_IS_BROKEN};
    read_completed_ = false;

    auto cb_wrapper = [&result, self{shared_from_this()}](
                          outcome::result<size_t> res) mutable {
      result.swap(res);
      self->read_completed_ = true;
      self->read_completed_cv_.notify_all();  // all for safety
    };

    auto buffer = std::make_shared<common::ByteArray>(kMaxFrameSize);
    raw_connection_->read(
        *buffer, kLenMarkerSize,
        [self{shared_from_this()}, buffer, cb{std::move(cb_wrapper)}](
            outcome::result<size_t> read_bytes_res) mutable {
          IO_OUTCOME_TRY(len_marker_size, read_bytes_res, cb)
          if (len_marker_size != kLenMarkerSize) {
            cb(Error::STREAM_IS_BROKEN);
            return;
          }
          uint32_t frame_len{
              ntohl(common::convert<uint32_t>(buffer->data()))};  // NOLINT
          if (frame_len > kMaxFrameSize) {
            cb(Error::OVERSIZED_FRAME);
            return;
          }
          self->raw_connection_->read(
              *buffer, frame_len,
              [self, buffer, frame_len,
               cb{cb}](outcome::result<size_t> read_bytes) mutable {
                IO_OUTCOME_TRY(read_frame_bytes, read_bytes, cb)
                if (frame_len != read_frame_bytes) {
                  cb(Error::STREAM_IS_BROKEN);
                  return;
                }
                IO_OUTCOME_TRY(mac_size, self->macSize(), cb)
                const auto data_size{frame_len - mac_size};
                auto data_span{gsl::make_span(buffer->data(), data_size)};
                auto mac_span{gsl::make_span(
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    buffer->data() + data_size, mac_size)};

                IO_OUTCOME_TRY(remote_mac, self->macRemote(data_span), cb)
                if (gsl::make_span(remote_mac) != mac_span) {
                  cb(Error::INVALID_MAC);
                  return;
                }
                IO_OUTCOME_TRY(decrypted_bytes, self->decryptRemote(data_span),
                               cb)
                size_t decrypted_bytes_len{decrypted_bytes.size()};
                for (auto &&e : decrypted_bytes) {
                  self->user_data_buffer_.emplace(std::forward<decltype(e)>(e));
                }
                cb(decrypted_bytes_len);
              });
        });

    std::unique_lock<std::mutex> lock(read_sync_mutex_);
    read_completed_cv_.wait(lock, [this] { return read_completed_.load(); });
    return result;
  }

  void SecioConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                              basic::Writer::WriteCallbackFunc cb) {
    if (!isInitialised()) {
      cb(Error::CONN_NOT_INITIALIZED);
    }
    std::scoped_lock<std::mutex> lock(write_mutex_);
    IO_OUTCOME_TRY(mac_size, macSize(), cb);
    size_t frame_len{bytes + mac_size};
    common::ByteArray frame_buffer;
    constexpr size_t len_field_size{sizeof(uint32_t)};
    frame_buffer.reserve(len_field_size + frame_len);

    common::putUint32BE(frame_buffer, frame_len);
    IO_OUTCOME_TRY(encrypted_data, encryptLocal(in), cb);
    IO_OUTCOME_TRY(mac_data, macLocal(encrypted_data), cb);
    frame_buffer.insert(frame_buffer.end(),
                        std::make_move_iterator(encrypted_data.begin()),
                        std::make_move_iterator(encrypted_data.end()));
    frame_buffer.insert(frame_buffer.end(),
                        std::make_move_iterator(mac_data.begin()),
                        std::make_move_iterator(mac_data.end()));
    raw_connection_->write(frame_buffer, frame_buffer.size(), std::move(cb));
  }

  void SecioConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                  basic::Writer::WriteCallbackFunc cb) {
    write(in, bytes, std::move(cb));
  }

  bool SecioConnection::isClosed() const {
    return raw_connection_->isClosed();
  }

  outcome::result<void> SecioConnection::close() {
    return raw_connection_->close();
  }

  outcome::result<size_t> SecioConnection::macSize() const {
    using HT = crypto::common::HashType;
    switch (hash_type_) {
      case HT::SHA256:
        return 32;
      case HT::SHA512:
        return 64;
      default:
        return Error::UNSUPPORTED_HASH;
    }
  }

  outcome::result<common::ByteArray> SecioConnection::macLocal(
      gsl::span<const uint8_t> message) const {
    return hmac_provider_->calculateDigest(
        hash_type_, local_stretched_key_.mac_key, message);
  }

  outcome::result<common::ByteArray> SecioConnection::macRemote(
      gsl::span<const uint8_t> message) const {
    return hmac_provider_->calculateDigest(
        hash_type_, remote_stretched_key_.mac_key, message);
  }

  outcome::result<common::ByteArray> SecioConnection::encryptLocal(
      gsl::span<const uint8_t> message) const {
    if (!isInitialised()) {
      return Error::CONN_NOT_INITIALIZED;
    }
    using CT = crypto::common::CipherType;
    switch (cipher_type_) {
      case CT::AES128:
        return aes_provider_->encryptAesCtr128(aes128_secrets_->local, message);
      case CT::AES256:
        return aes_provider_->encryptAesCtr256(aes256_secrets_->local, message);
      default:
        return Error::UNSUPPORTED_CIPHER;
    }
  }

  outcome::result<common::ByteArray> SecioConnection::decryptRemote(
      gsl::span<const uint8_t> message) const {
    if (!isInitialised()) {
      return Error::CONN_NOT_INITIALIZED;
    }
    using CT = crypto::common::CipherType;
    switch (cipher_type_) {
      case CT::AES128:
        return aes_provider_->decryptAesCtr128(aes128_secrets_->remote,
                                               message);
      case CT::AES256:
        return aes_provider_->decryptAesCtr256(aes256_secrets_->remote,
                                               message);
      default:
        return Error::UNSUPPORTED_CIPHER;
    }
  }
}  // namespace libp2p::connection
