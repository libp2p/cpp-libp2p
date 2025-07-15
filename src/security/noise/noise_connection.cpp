/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/noise_connection.hpp>

#include <libp2p/basic/read_return_size.hpp>
#include <libp2p/common/outcome_macro.hpp>
#include <libp2p/crypto/x25519_provider/x25519_provider_impl.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>

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

  void NoiseConnection::readSome(BytesOut out,
                                 libp2p::basic::Reader::ReadCallbackFunc cb) {
    if (not frame_buffer_->empty()) {
      auto n{std::min(out.size(), frame_buffer_->size())};
      auto begin{frame_buffer_->begin()};
      auto end{begin + static_cast<int64_t>(n)};
      std::copy(begin, end, out.begin());
      frame_buffer_->erase(begin, end);
      return cb(n);
    }
    framer_->read(
        [self{shared_from_this()}, out, cb{std::move(cb)}](
            outcome::result<std::shared_ptr<Bytes>> data_result) mutable {
          auto data = IF_ERROR_CB_RETURN(data_result);
          auto decrypted =
              IF_ERROR_CB_RETURN(self->decoder_cs_->decrypt({}, *data, {}));
          self->frame_buffer_->assign(decrypted.begin(), decrypted.end());
          self->readSome(out, std::move(cb));
        });
  }

  void NoiseConnection::writeSome(BytesIn in,
                                  basic::Writer::WriteCallbackFunc cb) {
    if (in.empty()) {
      cb(in.size());
      return;
    }
    if (in.size() > security::noise::kMaxPlainText) {
      in = in.first(security::noise::kMaxPlainText);
    }
    auto encrypted = IF_ERROR_CB_RETURN(encoder_cs_->encrypt({}, in, {}));
    // `InsecureReadWriter::write` doesn't leak `BytesIn` reference
    framer_->write(
        encrypted,
        [in, cb{std::move(cb)}](outcome::result<void> result) mutable {
          IF_ERROR_CB_RETURN(result);
          cb(in.size());
        });
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
}  // namespace libp2p::connection
