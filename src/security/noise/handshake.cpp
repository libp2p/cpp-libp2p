/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/handshake.hpp>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/security/noise/crypto/cipher_suite.hpp>
#include <libp2p/security/noise/crypto/noise_ccp1305.hpp>
#include <libp2p/security/noise/crypto/noise_dh.hpp>
#include <libp2p/security/noise/crypto/noise_sha256.hpp>
#include <libp2p/security/noise/crypto/state.hpp>
#include <libp2p/security/noise/handshake_message.hpp>
#include <libp2p/security/noise/handshake_message_marshaller_impl.hpp>

namespace libp2p::security::noise {

  std::shared_ptr<CipherSuite> defaultCipherSuite() {
    auto dh = std::make_shared<DiffieHellmanImpl>();
    auto hash = std::make_shared<SHA256>();
    auto cipher = std::make_shared<NamedCCPImpl>();
    return std::make_shared<CipherSuiteImpl>(std::move(dh), std::move(hash),
                                             std::move(cipher));
  }

  Handshake::Handshake(crypto::KeyPair local_key,
                       std::shared_ptr<connection::RawConnection> connection,
                       bool is_initiator)
      : local_key_{std::move(local_key)},
        conn_{std::move(connection)},
        initiator_{is_initiator},
        handshake_state_{std::make_unique<HandshakeState>()} {}

  outcome::result<std::vector<uint8_t>> Handshake::generateHandshakePayload(
      const DHKey &keypair) {
    const auto &prefix = kPayloadPrefix;
    const auto &pubkey = keypair.pub;
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

  outcome::result<void> Handshake::sendHandshakeMessage(
      gsl::span<const uint8_t> precompiled_out,
      gsl::span<const uint8_t> payload) {
    OUTCOME_TRY(write_result,
                handshake_state_->writeMessage(
                    precompiled_out.subspan(0, kLengthPrefixSize), payload));
    ByteArray buf;
    buf.reserve(write_result.data.size());
    common::putUint16BE(buf, write_result.data.size() - kLengthPrefixSize);
    auto begin = write_result.data.begin();
    std::advance(begin, kLengthPrefixSize);
    buf.insert(buf.end(), begin, write_result.data.end());
    conn_->write(buf, buf.size(),{}); // todo callback
    return outcome::success();
  }

  outcome::result<void> Handshake::runHandshake() {
    auto cipher_suite = defaultCipherSuite();
    OUTCOME_TRY(keypair, cipher_suite->generate());
    HandshakeStateConfig config(defaultCipherSuite(), handshakeXX(), initiator_,
                                keypair);
    OUTCOME_TRY(handshake_state_->init(std::move(config)));
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
