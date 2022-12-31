/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_HPP

#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/crypto/x25519_provider.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>
#include <libp2p/security/noise/crypto/state.hpp>
#include <libp2p/security/noise/handshake_message_marshaller.hpp>
#include <libp2p/security/noise/insecure_rw.hpp>
#include <libp2p/security/security_adaptor.hpp>

namespace libp2p::security::noise {

  std::shared_ptr<CipherSuite> defaultCipherSuite();

  class Handshake : public std::enable_shared_from_this<Handshake> {
   public:
    Handshake(
        std::shared_ptr<crypto::CryptoProvider> crypto_provider,
        std::unique_ptr<security::noise::HandshakeMessageMarshaller>
            noise_marshaller,
        crypto::KeyPair local_key,
        std::shared_ptr<connection::LayerConnection> connection,
        bool is_initiator, boost::optional<peer::PeerId> remote_peer_id,
        SecurityAdaptor::SecConnCallbackFunc cb,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller);

    void connect();

   private:
    const std::string kPayloadPrefix = "noise-libp2p-static-key:";

    void setCipherStates(std::shared_ptr<CipherState> cs1,
                         std::shared_ptr<CipherState> cs2);

    outcome::result<std::vector<uint8_t>> generateHandshakePayload(
        const DHKey &keypair);

    void sendHandshakeMessage(gsl::span<const uint8_t> payload,
                              basic::Writer::WriteCallbackFunc cb);

    void readHandshakeMessage(basic::MessageReadWriter::ReadCallbackFunc cb);

    outcome::result<void> handleRemoteHandshakePayload(
        gsl::span<const uint8_t> payload);

    outcome::result<void> runHandshake();

    // handshake callback
    void hscb(outcome::result<bool> secured);

    // constructor params
    std::shared_ptr<crypto::CryptoProvider> crypto_provider_;
    std::unique_ptr<security::noise::HandshakeMessageMarshaller>
        noise_marshaller_;
    const crypto::KeyPair local_key_;
    std::shared_ptr<connection::LayerConnection> conn_;
    bool initiator_;  /// false for incoming connections
    SecurityAdaptor::SecConnCallbackFunc connection_cb_;

    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;
    std::shared_ptr<ByteArray> read_buffer_;
    std::shared_ptr<InsecureReadWriter> rw_;

    // other params
    std::unique_ptr<HandshakeState> handshake_state_;

    std::shared_ptr<CipherState> enc_;
    std::shared_ptr<CipherState> dec_;
    boost::optional<peer::PeerId> remote_peer_id_;
    boost::optional<crypto::PublicKey> remote_peer_pubkey_;

    log::Logger log_ = log::createLogger("NoiseHandshake");
  };

}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_HPP
