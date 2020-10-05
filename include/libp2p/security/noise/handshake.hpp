/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_HPP
#define LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_HPP

#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/x25519_provider.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/security/noise/crypto/interfaces.hpp>
#include <libp2p/security/noise/handshake_message_marshaller.hpp>

namespace libp2p::security::noise {

  std::shared_ptr<CipherSuite> createCipherSuite();

  class Handshake {
   public:
    explicit Handshake(crypto::KeyPair local_key,
                       std::shared_ptr<connection::RawConnection> connection,
                       bool is_initiator);

   private:
    const std::string kPayloadPrefix = "noise-libp2p-static-key:";

    outcome::result<std::vector<uint8_t>> generateHandshakePayload(
        const crypto::x25519::Keypair &keypair);

    outcome::result<void> runHandshake();

    const crypto::KeyPair local_key_;
    std::shared_ptr<connection::RawConnection> conn_;
    bool initiator_;  /// false for incoming connections
    std::shared_ptr<crypto::CryptoProvider> crypto_provider_;  // todo init
    std::unique_ptr<security::noise::HandshakeMessageMarshaller>
        noise_marshaller_;  // todo init
  };

}  // namespace libp2p::security::noise

#endif  // LIBP2P_INCLUDE_LIBP2P_SECURITY_NOISE_HANDSHAKE_HPP
