/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/handshake.hpp>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/security/noise/crypto/cipher_suite.hpp>
#include <libp2p/security/noise/crypto/noise_ccp1305.hpp>
#include <libp2p/security/noise/crypto/noise_dh.hpp>
#include <libp2p/security/noise/crypto/noise_sha256.hpp>
#include <libp2p/security/noise/crypto/state.hpp>
#include <libp2p/security/noise/handshake_message.hpp>
#include <libp2p/security/noise/handshake_message_marshaller_impl.hpp>

#ifndef UNIQUE_NAME
#define UNIQUE_NAME(base) base##__LINE__
#endif  // UNIQUE_NAME

#define IO_OUTCOME_TRY_NAME(var, val, res, cb) \
  auto && (var) = (res);                       \
  if ((var).has_error()) {                     \
    cb((var).error());                         \
    return;                                    \
  }                                            \
  auto && (val) = (var).value();

#define IO_OUTCOME_TRY(name, res, cb) \
  IO_OUTCOME_TRY_NAME(UNIQUE_NAME(name), name, res, cb)

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
        read_buffer_{std::make_shared<ByteArray>(kMaxMsgLen)},
        rw_{conn_, read_buffer_},
        handshake_state_{std::make_unique<HandshakeState>()} {
    read_buffer_->resize(kMaxMsgLen);
  }

  void Handshake::setCipherStates(std::shared_ptr<CipherState> cs1,
                                  std::shared_ptr<CipherState> cs2) {
    if (initiator_) {
      enc_ = std::move(cs1);
      dec_ = std::move(cs2);
    } else {
      enc_ = std::move(cs2);
      dec_ = std::move(cs1);
    }
  }

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

  void Handshake::sendHandshakeMessage(gsl::span<const uint8_t> payload,
                                       basic::Writer::WriteCallbackFunc cb) {
    IO_OUTCOME_TRY(write_result, handshake_state_->writeMessage({}, payload),
                   cb);
    auto write_cb =
        [self{shared_from_this()}, cb{std::move(cb)},
         wr{std::move(write_result)}](outcome::result<size_t> result) {
          IO_OUTCOME_TRY(bytes_written, result, cb);
          if (wr.cs1 and wr.cs2) {
            self->setCipherStates(wr.cs1, wr.cs2);
          }
          cb(bytes_written);
        };
    rw_.write(write_result.data, write_cb);
  }

  void Handshake::readHandshakeMessage(basic::Reader::ReadCallbackFunc cb) {
    auto read_cb = [self{shared_from_this()}, cb{std::move(cb)}](auto result) {
      IO_OUTCOME_TRY(buffer, result, cb);
      IO_OUTCOME_TRY(rr, self->handshake_state_->readMessage({}, *buffer), cb);
      if (rr.cs1 and rr.cs2) {
        self->setCipherStates(rr.cs1, rr.cs2);
      }
      cb(rr.data.size());
    };
    rw_.read(read_cb);
  }

  outcome::result<void> Handshake::handleRemoteHandshakePayload(
      gsl::span<const uint8_t> payload) {
    OUTCOME_TRY(remote_payload, noise_marshaller_->unmarshal(payload));
    OUTCOME_TRY(remote_id, peer::PeerId::fromPublicKey(remote_payload.second));
    auto &&handy_payload = remote_payload.first;
    if (initiator_ and remote_peer_id_ != remote_id) {
      // todo log expected and received
      return std::errc::bad_address;
    }
    ByteArray to_verify;
    to_verify.reserve(kPayloadPrefix.size()
                      + handy_payload.identity_key.data.size());
    std::copy(kPayloadPrefix.begin(), kPayloadPrefix.end(),
              std::back_inserter(to_verify));
    OUTCOME_TRY(remote_static, handshake_state_->remotePeerStaticPubkey());
    std::copy(remote_static.begin(), remote_static.end(),
              std::back_inserter(to_verify));
    OUTCOME_TRY(signature_correct,
                crypto_provider_->verify(to_verify, handy_payload.identity_sig,
                                         handy_payload.identity_key));
    if (not signature_correct) {
      return std::errc::owner_dead;  // todo
    }
    remote_peer_id_ = remote_id;
    remote_peer_pubkey_ = handy_payload.identity_key;
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
//    if (initiator_) {
//      sendHandshakeMessage({}, [self{shared_from_this()}](auto result) {
////        IO_OUTCOME_TRY(bytes_sent, result, cb)
//      });
//    } else {
//    }

    return outcome::success();
  }

}  // namespace libp2p::security::noise
