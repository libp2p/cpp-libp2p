/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/noise_connection.hpp>

#include <libp2p/crypto/x25519_provider/x25519_provider_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, NoiseConnection::Error, e) {
  using E = libp2p::connection::NoiseConnection::Error;
  switch (e) {
    case E::FAILURE:
      return "failure";
    default:
      return "Unknown error";
  }
}

#define _OUTCOME_CB1(t, r) \
  auto &&t{r};             \
  if (!t)                  \
    return cb(t.error());
#define _OUTCOME_CB(t, l, r) \
  _OUTCOME_CB1(t, r)         \
  auto &&l{r.value()};
#define OUTCOME_CB(l, r) _OUTCOME_CB(BOOST_OUTCOME_TRY_UNIQUE_NAME, l, r)

namespace libp2p::connection {
  NoiseConnection::NoiseConnection(
      std::shared_ptr<RawConnection> raw_connection,
      crypto::PublicKey localPubkey, crypto::PublicKey remotePubkey,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      std::shared_ptr<security::noise::CipherState> cipher)
      : raw_connection_{std::move(raw_connection)},
        local_{std::move(localPubkey)},
        remote_{std::move(remotePubkey)},
        key_marshaller_{std::move(key_marshaller)},
        cipher_{std::move(cipher)},
        frame_buffer_{
            std::make_shared<common::ByteArray>(security::noise::kMaxMsgLen)},
        framer_{std::make_shared<security::noise::InsecureReadWriter>(
            raw_connection_, frame_buffer_)},
        already_read_{0},
        already_wrote_{0} {
    BOOST_ASSERT(raw_connection_);
    BOOST_ASSERT(key_marshaller_);
  }

  bool NoiseConnection::isClosed() const {
    return raw_connection_->isClosed();
  }

  outcome::result<void> NoiseConnection::close() {
    return raw_connection_->close();
  }

  void NoiseConnection::read(gsl::span<uint8_t> out, size_t bytes,
                             libp2p::basic::Reader::ReadCallbackFunc cb) {
    size_t out_size{out.empty() ? 0u : static_cast<size_t>(out.size())};
    BOOST_ASSERT(out_size >= bytes);
    if (bytes == 0) {
      auto n{already_read_};
      already_read_ = 0;
      return cb(n);
    }
    readSome(
        out, bytes,
        [self{shared_from_this()}, out, bytes, cb{std::move(cb)}](auto _n) {
          OUTCOME_CB(n, _n);
          self->already_read_ += n;
          self->read(out.subspan(n), bytes - n, std::move(cb));
        });
  }

  void NoiseConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                 libp2p::basic::Reader::ReadCallbackFunc cb) {
    if (!frame_buffer_->empty()) {
      auto n{std::min(bytes, frame_buffer_->size())};
      auto begin{frame_buffer_->begin()}, end{begin + n};
      std::copy(begin, end, out.begin());
      frame_buffer_->erase(begin, end);
      return cb(n);
    }
    framer_->read(
        [self{shared_from_this()}, out, bytes, cb{std::move(cb)}](auto _data) {
          OUTCOME_CB(data, _data);
          // TODO: remove precompiled_out
          OUTCOME_CB(decrypted, self->cipher_->decrypt({}, *data, {}));
          self->frame_buffer_->assign(decrypted.begin(), decrypted.end());
          self->readSome(out, bytes, std::move(cb));
        });
  }

  void NoiseConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                              libp2p::basic::Writer::WriteCallbackFunc cb) {
    if (bytes == 0) {
      auto n{already_wrote_};
      already_wrote_ = 0;
      return cb(n);
    }
    auto n{std::min(bytes, security::noise::kMaxPlainText)};
    // TODO: remove precompiled_out
    OUTCOME_CB(encrypted, cipher_->encrypt({}, in.subspan(0, n), {}));
    writing_ = std::move(encrypted);
    framer_->write(writing_,
                   [self{shared_from_this()}, in{in.subspan(n)},
                    bytes{bytes - n}, cb{std::move(cb)}](auto _n) {
                     OUTCOME_CB(n, _n);
                     self->already_wrote_ += n;
                     self->write(in, bytes, std::move(cb));
                   });
  }

  void NoiseConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                  libp2p::basic::Writer::WriteCallbackFunc cb) {
    write(in, bytes, std::move(cb));
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
