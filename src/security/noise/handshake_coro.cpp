/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <libp2p/security/noise/handshake_coro.hpp>

#include <libp2p/common/byteutil.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/security/noise/crypto/cipher_suite.hpp>
#include <libp2p/security/noise/crypto/noise_ccp1305.hpp>
#include <libp2p/security/noise/crypto/noise_dh.hpp>
#include <libp2p/security/noise/crypto/noise_sha256.hpp>
#include <libp2p/security/noise/crypto/state.hpp>
#include <libp2p/security/noise/handshake.hpp>
#include <libp2p/security/noise/handshake_message.hpp>
#include <libp2p/security/noise/noise_connection.hpp>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace libp2p::security::noise {

  namespace {
    template <typename T>
    void unused(T &&) {}
  }  // namespace

  HandshakeCoro::HandshakeCoro(
      std::shared_ptr<crypto::CryptoProvider> crypto_provider,
      std::unique_ptr<security::noise::HandshakeMessageMarshaller>
          noise_marshaller,
      crypto::KeyPair local_key,
      std::shared_ptr<connection::LayerConnection> connection,
      bool is_initiator,
      boost::optional<peer::PeerId> remote_peer_id,
      std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller)
      : crypto_provider_{std::move(crypto_provider)},
        noise_marshaller_{std::move(noise_marshaller)},
        local_key_{std::move(local_key)},
        conn_{std::move(connection)},
        initiator_{is_initiator},
        key_marshaller_{std::move(key_marshaller)},
        read_buffer_{std::make_shared<Bytes>(kMaxMsgLen)},
        rw_{std::make_shared<InsecureReadWriter>(conn_, read_buffer_)},
        handshake_state_{std::make_unique<HandshakeState>()},
        remote_peer_id_{std::move(remote_peer_id)} {
    read_buffer_->resize(kMaxMsgLen);
  }

  boost::asio::awaitable<
      outcome::result<std::shared_ptr<connection::SecureConnection>>>
  HandshakeCoro::connect() {
    return runHandshake();
  }

  void HandshakeCoro::setCipherStates(std::shared_ptr<CipherState> cs1,
                                      std::shared_ptr<CipherState> cs2) {
    if (initiator_) {
      enc_ = std::move(cs1);
      dec_ = std::move(cs2);
    } else {
      enc_ = std::move(cs2);
      dec_ = std::move(cs1);
    }
  }

  outcome::result<std::vector<uint8_t>> HandshakeCoro::generateHandshakePayload(
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

  boost::asio::awaitable<outcome::result<size_t>>
  HandshakeCoro::sendHandshakeMessage(BytesIn payload) {
    auto write_result = handshake_state_->writeMessage({}, payload);
    if (write_result.has_error()) {
      co_return write_result.error();
    }

    auto write_bytes_result = co_await rw_->write(write_result.value().data);
    if (write_bytes_result.has_error()) {
      co_return write_bytes_result.error();
    }

    if (write_result.value().cs1 && write_result.value().cs2) {
      setCipherStates(write_result.value().cs1, write_result.value().cs2);
    }

    co_return write_bytes_result.value();
  }

  boost::asio::awaitable<outcome::result<std::shared_ptr<Bytes>>>
  HandshakeCoro::readHandshakeMessage() {
    auto read_result = co_await rw_->read();
    if (read_result.has_error()) {
      co_return read_result.error();
    }

    auto buffer = read_result.value();
    auto read_message_result = handshake_state_->readMessage({}, *buffer);
    if (read_message_result.has_error()) {
      co_return read_message_result.error();
    }

    if (read_message_result.value().cs1 && read_message_result.value().cs2) {
      setCipherStates(read_message_result.value().cs1,
                      read_message_result.value().cs2);
    }

    auto shared_data =
        std::make_shared<Bytes>(std::move(read_message_result.value().data));
    co_return shared_data;
  }

  outcome::result<void> HandshakeCoro::handleRemoteHandshakePayload(
      BytesIn payload) {
    OUTCOME_TRY(remote_payload, noise_marshaller_->unmarshal(payload));
    OUTCOME_TRY(remote_id, peer::PeerId::fromPublicKey(remote_payload.second));
    auto &&handy_payload = remote_payload.first;
    if (initiator_ && remote_peer_id_ != remote_id) {
      SL_DEBUG(log_,
               "Remote peer id mismatches already known, expected {}, got {}",
               remote_peer_id_->toHex(),
               remote_id.toHex());
      return std::errc::bad_address;
    }
    Bytes to_verify;
    to_verify.reserve(kPayloadPrefix.size()
                      + handy_payload.identity_key.data.size());
    std::copy(kPayloadPrefix.begin(),
              kPayloadPrefix.end(),
              std::back_inserter(to_verify));
    OUTCOME_TRY(remote_static, handshake_state_->remotePeerStaticPubkey());
    std::copy(remote_static.begin(),
              remote_static.end(),
              std::back_inserter(to_verify));
    OUTCOME_TRY(
        signature_correct,
        crypto_provider_->verify(
            to_verify, handy_payload.identity_sig, handy_payload.identity_key));
    if (!signature_correct) {
      SL_TRACE(log_, "Remote peer's payload signature verification failed");
      return std::errc::owner_dead;
    }
    remote_peer_id_ = remote_id;
    remote_peer_pubkey_ = handy_payload.identity_key;
    return outcome::success();
  }

  boost::asio::awaitable<
      outcome::result<std::shared_ptr<connection::SecureConnection>>>
  HandshakeCoro::runHandshake() {
    auto cipher_suite = defaultCipherSuite();

    auto keypair_res = cipher_suite->generate();
    if (keypair_res.has_error()) {
      co_return keypair_res.error();
    }
    auto keypair = keypair_res.value();

    HandshakeStateConfig config(
        defaultCipherSuite(), handshakeXX, initiator_, keypair);
    auto init_result = handshake_state_->init(std::move(config));
    if (init_result.has_error()) {
      co_return init_result.error();
    }

    auto payload_res = generateHandshakePayload(keypair);
    if (payload_res.has_error()) {
      co_return payload_res.error();
    }
    auto payload = payload_res.value();

    if (initiator_) {
      // Outgoing connection. Stage 0
      SL_TRACE(log_, "outgoing connection. stage 0");

      auto send_result = co_await sendHandshakeMessage({});
      if (send_result.has_error()) {
        co_return send_result.error();
      }

      if (0 == send_result.value()) {
        co_return std::errc::bad_message;
      }

      // Outgoing connection. Stage 1
      SL_TRACE(log_, "outgoing connection. stage 1");

      auto read_result = co_await readHandshakeMessage();
      if (read_result.has_error()) {
        co_return read_result.error();
      }

      auto handle_result = handleRemoteHandshakePayload(*read_result.value());
      if (handle_result.has_error()) {
        co_return handle_result.error();
      }

      // Outgoing connection. Stage 2
      SL_TRACE(log_, "outgoing connection. stage 2");

      auto send_payload_result = co_await sendHandshakeMessage(payload);
      if (send_payload_result.has_error()) {
        co_return send_payload_result.error();
      }

    } else {
      // Incoming connection. Stage 0
      SL_TRACE(log_, "incoming connection. stage 0");

      auto read_result = co_await readHandshakeMessage();
      if (read_result.has_error()) {
        co_return read_result.error();
      }

      // Incoming connection. Stage 1
      SL_TRACE(log_, "incoming connection. stage 1");

      auto send_result = co_await sendHandshakeMessage(payload);
      if (send_result.has_error()) {
        co_return send_result.error();
      }

      // Incoming connection. Stage 2
      SL_TRACE(log_, "incoming connection. stage 2");

      auto read_payload_result = co_await readHandshakeMessage();
      if (read_payload_result.has_error()) {
        co_return read_payload_result.error();
      }

      auto handle_result =
          handleRemoteHandshakePayload(*read_payload_result.value());
      if (handle_result.has_error()) {
        co_return handle_result.error();
      }
    }

    if (!remote_peer_pubkey_) {
      log_->error("Remote peer static pubkey remains unknown");
      co_return std::errc::connection_aborted;
    }

    auto secured_connection = std::make_shared<connection::NoiseConnection>(
        conn_,
        local_key_.publicKey,
        remote_peer_pubkey_.value(),
        key_marshaller_,
        enc_,
        dec_);
    log_->info("Handshake succeeded");
    co_return secured_connection;
  }

}  // namespace libp2p::security::noise
