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

namespace libp2p::connection {
  NoiseConnection::NoiseConnection(
      crypto::KeyPair keypair, std::shared_ptr<RawConnection> raw_connection,
      crypto::PublicKey localPubkey, crypto::PublicKey remotePubkey,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
      std::shared_ptr<crypto::CryptoProvider> crypto_provider)
      : keypair_{std::move(keypair)},
        raw_connection_{std::move(raw_connection)},
        local_{std::move(localPubkey)},
        remote_{std::move(remotePubkey)},
        key_marshaller_{std::move(key_marshaller)},
        crypto_provider_{std::move(crypto_provider)},
        noise_marshaller_{
            std::make_unique<security::noise::HandshakeMessageMarshallerImpl>(
                key_marshaller_)} {
    BOOST_ASSERT(raw_connection_);
    BOOST_ASSERT(key_marshaller_);
    BOOST_ASSERT(crypto_provider_);
    BOOST_ASSERT(noise_marshaller_);
  }

  bool NoiseConnection::isClosed() const {
    return raw_connection_->isClosed();
  }

  outcome::result<void> NoiseConnection::close() {
    return raw_connection_->close();
  }

  void NoiseConnection::read(gsl::span<uint8_t> out, size_t bytes,
                             libp2p::basic::Reader::ReadCallbackFunc cb) {}

  void NoiseConnection::readSome(gsl::span<uint8_t> out, size_t bytes,
                                 libp2p::basic::Reader::ReadCallbackFunc cb) {}

  void NoiseConnection::write(gsl::span<const uint8_t> in, size_t bytes,
                              libp2p::basic::Writer::WriteCallbackFunc cb) {}

  void NoiseConnection::writeSome(gsl::span<const uint8_t> in, size_t bytes,
                                  libp2p::basic::Writer::WriteCallbackFunc cb) {
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

  outcome::result<void> NoiseConnection::runHandshake() {
    std::unique_ptr<crypto::x25519::X25519Provider> x25519provider =
        std::make_unique<crypto::x25519::X25519ProviderImpl>();

    OUTCOME_TRY(keypair, x25519provider->generate());
    OUTCOME_TRY(payload, generateHandshakePayload(keypair));

    const size_t dh25519_len = 32;
    const size_t poly1305_tag_size = 16;
    const size_t length_prefix_size = 2;
    size_t max_msg_size =
        2 * dh25519_len + payload.size() + 2 * poly1305_tag_size;
    std::vector<uint8_t> buffer(max_msg_size + length_prefix_size);

    if (isInitiator()) {
    }

    return outcome::success();
  }

  outcome::result<std::vector<uint8_t>>
  NoiseConnection::generateHandshakePayload(
      const crypto::x25519::Keypair &keypair) {
    const auto &prefix = payload_signature_prefix_;
    const auto &pubkey = keypair.public_key;
    std::vector<uint8_t> to_sign;
    to_sign.reserve(prefix.size() + pubkey.size());
    std::copy(prefix.begin(), prefix.end(), std::back_inserter(to_sign));
    std::copy(pubkey.begin(), pubkey.end(), std::back_inserter(to_sign));

    OUTCOME_TRY(signed_payload,
                crypto_provider_->sign(to_sign, keypair_.privateKey));
    security::noise::HandshakeMessage payload{
        .identity_key = keypair_.publicKey,
        .identity_sig = std::move(signed_payload),
        .data = {}};
    return noise_marshaller_->marshal(payload);
  }

}  // namespace libp2p::connection
