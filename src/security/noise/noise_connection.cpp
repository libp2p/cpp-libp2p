/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/noise_connection.hpp>

#include <libp2p/common/span_size.hpp>
#include <libp2p/crypto/x25519_provider/x25519_provider_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

#ifndef UNIQUE_NAME
#define UNIQUE_NAME(base) base##__LINE__
#endif  // UNIQUE_NAME

#define OUTCOME_CB_I(var, res) \
  auto && (var) = (res);       \
  if ((var).has_error()) {     \
    return cb((var).error());  \
  }

#define OUTCOME_CB_NAME_I(var, val, res) \
  OUTCOME_CB_I(var, res)                 \
  auto && (val) = (var).value();

#define OUTCOME_CB(name, res) OUTCOME_CB_NAME_I(UNIQUE_NAME(name), name, res)

namespace libp2p::connection {
  NoiseConnection::NoiseConnection(
      std::shared_ptr<RawConnection> raw_connection,
      crypto::PublicKey localPubkey, crypto::PublicKey remotePubkey,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      std::shared_ptr<security::noise::CipherState> encoder,
      std::shared_ptr<security::noise::CipherState> decoder)
      : raw_connection_{std::move(raw_connection)},
        local_{std::move(localPubkey)},
        remote_{std::move(remotePubkey)},
        key_marshaller_{std::move(key_marshaller)},
        encoder_cs_{std::move(encoder)},
        decoder_cs_{std::move(decoder)},
        frame_buffer_{
            std::make_shared<common::ByteArray>(security::noise::kMaxMsgLen)},
        framer_{std::make_shared<security::noise::InsecureReadWriter>(
            raw_connection_, frame_buffer_)} {
    BOOST_ASSERT(raw_connection_);
    BOOST_ASSERT(key_marshaller_);
    BOOST_ASSERT(encoder_cs_);
    BOOST_ASSERT(decoder_cs_);
    BOOST_ASSERT(frame_buffer_);
    BOOST_ASSERT(framer_);
    frame_buffer_->resize(0);
  }

  bool NoiseConnection::isClosed() const {
    return raw_connection_->isClosed();
  }

  outcome::result<void> NoiseConnection::close() {
    return raw_connection_->close();
  }

  void NoiseConnection::read(gsl::span<uint8_t> out, size_t bytes,
                             libp2p::basic::Reader::ReadCallbackFunc cb) {
    BOOST_ASSERT(spanSize(out) >= bytes);
    readFull(out.first(gsl::narrow<ptrdiff_t>(bytes)), bytes, std::move(cb));
  }

  void NoiseConnection::readFull(gsl::span<uint8_t> out, size_t total,
                                 ReadCallbackFunc cb) {
    readSome(out, out.size(),
             [self{shared_from_this()}, out, total,
              cb{std::move(cb)}](auto _n) mutable {
               OUTCOME_CB(n, _n);
               BOOST_ASSERT(n <= spanSize(out));
               if (n == spanSize(out)) {
                 return cb(total);
               }
               self->readFull(out.subspan(n), total, std::move(cb));
             });
  }

  void NoiseConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                 libp2p::basic::Reader::ReadCallbackFunc cb) {
    if (not frame_buffer_->empty()) {
      auto n{std::min(bytes, frame_buffer_->size())};
      auto begin{frame_buffer_->begin()};
      auto end{begin + gsl::narrow<ptrdiff_t>(n)};
      std::copy(begin, end, out.begin());
      frame_buffer_->erase(begin, end);
      return cb(n);
    }
    framer_->read([self{shared_from_this()}, out, bytes,
                   cb{std::move(cb)}](auto _data) mutable {
      OUTCOME_CB(data, _data);
      OUTCOME_CB(decrypted, self->decoder_cs_->decrypt({}, *data, {}));
      self->frame_buffer_->assign(decrypted.begin(), decrypted.end());
      self->readSome(out, bytes, std::move(cb));
    });
  }

  void NoiseConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                              libp2p::basic::Writer::WriteCallbackFunc cb) {
    BOOST_ASSERT(spanSize(in) >= bytes);
    writeFull(in.first(gsl::narrow<ptrdiff_t>(bytes)), bytes, std::move(cb));
  }

  void NoiseConnection::writeFull(gsl::span<const uint8_t> in, size_t total,
                                  basic::Writer::WriteCallbackFunc cb) {
    writeSome(in, in.size(),
              [self{shared_from_this()}, in, total,
               cb{std::move(cb)}](auto _n) mutable {
                OUTCOME_CB(n, _n);
                BOOST_ASSERT(n <= spanSize(in));
                if (n == spanSize(in)) {
                  return cb(total);
                }
                self->writeFull(in.subspan(n), total, std::move(cb));
              });
  }

  void NoiseConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                  libp2p::basic::Writer::WriteCallbackFunc cb) {
    const auto n{std::min(bytes, security::noise::kMaxPlainText)};
    if (n == 0) {
      return cb(n);
    }
    OUTCOME_CB(encrypted, encoder_cs_->encrypt({}, in.subspan(0, n), {}));
    writing_ = std::move(encrypted);
    framer_->write(writing_, [cb{std::move(cb)}, n](auto _n) {
      if (!_n) {
        return cb(_n.error());
      }
      cb(n);
    });
  }

  void NoiseConnection::deferReadCallback(outcome::result<size_t> res,
                                          ReadCallbackFunc cb) {
    raw_connection_->deferReadCallback(res, std::move(cb));
  }

  void NoiseConnection::deferWriteCallback(std::error_code ec,
                                           WriteCallbackFunc cb) {
    raw_connection_->deferWriteCallback(ec, std::move(cb));
  }

  bool NoiseConnection::isInitiator() const noexcept {
    return raw_connection_->isInitiator();
  }

  outcome::result<libp2p::multi::Multiaddress>
  NoiseConnection::localMultiaddr() {
    return raw_connection_->localMultiaddr();
  }

  outcome::result<libp2p::multi::Multiaddress>
  NoiseConnection::remoteMultiaddr() {
    return raw_connection_->remoteMultiaddr();
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
}  // namespace libp2p::connection
