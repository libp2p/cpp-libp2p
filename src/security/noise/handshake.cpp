/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/handshake.hpp>

#include <libp2p/crypto/x25519_provider/x25519_provider_impl.hpp>
#include <libp2p/security/noise/handshake_message.hpp>
#include <libp2p/security/noise/handshake_message_marshaller_impl.hpp>
#include <libp2p/security/noise/crypto/state.hpp>

namespace libp2p::security::noise {

  std::shared_ptr<CipherSuite> createCipherSuite() {
    return std::shared_ptr<CipherSuite>();
  }

  Handshake::Handshake(crypto::KeyPair local_key,
                       std::shared_ptr<connection::RawConnection> connection,
                       bool is_initiator)
      : local_key_{std::move(local_key)},
        conn_{std::move(connection)},
        initiator_{is_initiator} {}

  outcome::result<std::vector<uint8_t>> Handshake::generateHandshakePayload(
      const crypto::x25519::Keypair &keypair) {
    const auto &prefix = kPayloadPrefix;
    const auto &pubkey = keypair.public_key;
    std::vector<uint8_t> to_sign;
    to_sign.reserve(prefix.size() + pubkey.size());
    std::copy(prefix.begin(), prefix.end(), std::back_inserter(to_sign));
    std::copy(pubkey.begin(), pubkey.end(), std::back_inserter(to_sign));

    OUTCOME_TRY(signed_payload,
                crypto_provider_->sign(to_sign, local_key_.privateKey));
    security::noise::HandshakeMessage payload{
        .identity_key = local_key_.publicKey,
        .identity_sig = std::move(signed_payload),
        .data = {}};
    return noise_marshaller_->marshal(payload);
  }

  outcome::result<void> Handshake::runHandshake() {
    std::unique_ptr<crypto::x25519::X25519Provider> x25519provider =
        std::make_unique<crypto::x25519::X25519ProviderImpl>();

    // todo ? handshake state

    OUTCOME_TRY(keypair, x25519provider->generate());
    OUTCOME_TRY(payload, generateHandshakePayload(keypair));

    const size_t dh25519_len = 32;
    const size_t poly1305_tag_size = 16;
    const size_t length_prefix_size = 2;
    size_t max_msg_size =
        2 * dh25519_len + payload.size() + 2 * poly1305_tag_size;
    std::vector<uint8_t> buffer(max_msg_size + length_prefix_size);
    if (initiator_) {

    } else {

    }

    return outcome::success();
  }

}  // namespace libp2p::security::noise
