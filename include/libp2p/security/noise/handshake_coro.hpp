/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/crypto/x25519_provider.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>
#include <libp2p/security/noise/crypto/state.hpp>
#include <libp2p/security/noise/handshake_message_marshaller.hpp>
#include <libp2p/security/noise/insecure_rw.hpp>
#include <libp2p/security/security_adaptor.hpp>

#include <boost/asio/awaitable.hpp>

namespace libp2p::security::noise {

  /**
   * Coroutine version of the Noise handshake protocol.
   */
  class HandshakeCoro : public std::enable_shared_from_this<HandshakeCoro> {
   public:
    HandshakeCoro(
        std::shared_ptr<crypto::CryptoProvider> crypto_provider,
        std::unique_ptr<security::noise::HandshakeMessageMarshaller> noise_marshaller,
        crypto::KeyPair local_key,
        std::shared_ptr<connection::LayerConnection> connection,
        bool is_initiator,
        boost::optional<peer::PeerId> remote_peer_id,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller);

    /**
     * Performs the handshake process using coroutines
     * @return awaitable with secure connection on success or error
     */
    boost::asio::awaitable<outcome::result<std::shared_ptr<connection::SecureConnection>>> connect();

   private:
    const std::string kPayloadPrefix = "noise-libp2p-static-key:";

    void setCipherStates(std::shared_ptr<CipherState> cs1,
                        std::shared_ptr<CipherState> cs2);

    outcome::result<std::vector<uint8_t>> generateHandshakePayload(
        const DHKey &keypair);

    boost::asio::awaitable<outcome::result<size_t>> sendHandshakeMessage(
        BytesIn payload);

    boost::asio::awaitable<outcome::result<std::shared_ptr<Bytes>>> readHandshakeMessage();

    outcome::result<void> handleRemoteHandshakePayload(BytesIn payload);

    boost::asio::awaitable<outcome::result<std::shared_ptr<connection::SecureConnection>>> runHandshake();

    // constructor params
    std::shared_ptr<crypto::CryptoProvider> crypto_provider_;
    std::unique_ptr<security::noise::HandshakeMessageMarshaller> noise_marshaller_;
    const crypto::KeyPair local_key_;
    std::shared_ptr<connection::LayerConnection> conn_;
    bool initiator_;  /// false for incoming connections
    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;
    std::shared_ptr<Bytes> read_buffer_;
    std::shared_ptr<InsecureReadWriter> rw_;

    // other params
    std::unique_ptr<HandshakeState> handshake_state_;
    std::shared_ptr<CipherState> enc_;
    std::shared_ptr<CipherState> dec_;
    boost::optional<peer::PeerId> remote_peer_id_;
    boost::optional<crypto::PublicKey> remote_peer_pubkey_;

    log::Logger log_ = log::createLogger("NoiseHandshakeCoro");
  };

}  // namespace libp2p::security::noise
