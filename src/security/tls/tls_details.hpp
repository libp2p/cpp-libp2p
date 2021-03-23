/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECURITY_TLS_DETAILS_HPP
#define LIBP2P_SECURITY_TLS_DETAILS_HPP

#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace libp2p::security::tls_details {

  /// Returns "tls" logger
  log::Logger log();

  /// Peer certificate verify helper. Allows self-signed certificates to pass
  /// \param status preverified status
  /// \param ctx asio wrapper around X509_STORE_CTX
  /// \return verify result
  bool verifyCallback(bool status, boost::asio::ssl::verify_context &ctx);

  struct CertificateAndKey {
    /// self-signed certificate in ASN1 DER format
    std::vector<uint8_t> certificate;

    /// private key in ASN1 DER format
    std::array<uint8_t, 121> private_key{};
  };

  /// Creates self-signed certificate with libp2p-specific extension
  /// \param host_key_pair key pair of this host
  /// \param key_marshaller key marshaller needed to construct the extension
  /// \return generated cert and key, throws on failure
  CertificateAndKey makeCertificate(
      const crypto::KeyPair &host_key_pair,
      const crypto::marshaller::KeyMarshaller &key_marshaller);

  struct PubkeyAndPeerId {
    /// remote peer's public key
    crypto::PublicKey public_key;

    /// remote peer id
    peer::PeerId peer_id;
  };

  /// Extract libp2p-special extension from certificate which contains
  /// peer's public key. Also verifies signature using that key
  /// \param peer_certificate remote peer's certificate
  /// \param key_marshaller key marshaller (needed to deal with extension data)
  /// \return pubkey and peer id, or error
  outcome::result<PubkeyAndPeerId> verifyPeerAndExtractIdentity(
      X509 *peer_certificate,
      const crypto::marshaller::KeyMarshaller &key_marshaller);

}  // namespace libp2p::security::tls_details

#endif  // LIBP2P_SECURITY_TLS_DETAILS_HPP
