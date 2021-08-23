/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECIO_ADAPTOR_HPP
#define LIBP2P_SECIO_ADAPTOR_HPP

#include <libp2p/crypto/aes_ctr.hpp>
#include <libp2p/crypto/crypto_provider.hpp>
#include <libp2p/crypto/hmac_provider.hpp>
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/crypto/random_generator.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/security/secio/exchange_message_marshaller.hpp>
#include <libp2p/security/secio/propose_message_marshaller.hpp>
#include <libp2p/security/security_adaptor.hpp>

namespace libp2p::security {

  namespace secio {
    class Dialer;
  }

  /**
   * Implementation of security adaptor, which creates SECIO connection.
   */
  class Secio : public SecurityAdaptor,
                public std::enable_shared_from_this<Secio> {
   public:
    enum class Error {
      REMOTE_PEER_SIGNATURE_IS_INVALID = 1,
      INITIAL_PACKET_VERIFICATION_FAILED,
    };

    static constexpr auto kProtocolId = "/secio/1.0.0";
    static constexpr auto kExchanges = "P-256,P-384,P-521";
    static constexpr auto kCiphers = "AES-256,AES-128";
    static constexpr auto kHashes = "SHA256,SHA512";

    ~Secio() override = default;

    Secio(std::shared_ptr<crypto::random::CSPRNG> csprng,
          std::shared_ptr<crypto::CryptoProvider> crypto_provider,
          std::shared_ptr<secio::ProposeMessageMarshaller> propose_marshaller,
          std::shared_ptr<secio::ExchangeMessageMarshaller> exchange_marshaller,
          std::shared_ptr<peer::IdentityManager> idmgr,
          std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller,
          std::shared_ptr<crypto::hmac::HmacProvider> hmac_provider);

    peer::Protocol getProtocolId() const override;

    void secureInbound(std::shared_ptr<connection::RawConnection> inbound,
                       SecConnCallbackFunc cb) override;

    void secureOutbound(std::shared_ptr<connection::RawConnection> outbound,
                        const peer::PeerId &p, SecConnCallbackFunc cb) override;

   private:
    void sendProposeMessage(
        const std::shared_ptr<connection::RawConnection> &conn,
        const std::shared_ptr<secio::Dialer> &dialer,
        SecConnCallbackFunc cb) const;

    void receiveProposeMessage(
        const std::shared_ptr<connection::RawConnection> &conn,
        const std::shared_ptr<secio::Dialer> &dialer,
        SecConnCallbackFunc cb) const;

    void sendExchangeMessage(
        const std::shared_ptr<connection::RawConnection> &conn,
        const std::shared_ptr<secio::Dialer> &dialer,
        SecConnCallbackFunc cb) const;

    void receiveExchangeMessage(
        const std::shared_ptr<connection::RawConnection> &conn,
        const std::shared_ptr<secio::Dialer> &dialer,
        SecConnCallbackFunc cb) const;

    void closeConnection(
        const std::shared_ptr<libp2p::connection::RawConnection> &conn,
        const std::error_code &err) const;

    std::shared_ptr<crypto::random::CSPRNG> csprng_;
    std::shared_ptr<crypto::CryptoProvider> crypto_provider_;
    std::shared_ptr<secio::ProposeMessageMarshaller> propose_marshaller_;
    std::shared_ptr<secio::ExchangeMessageMarshaller> exchange_marshaller_;
    std::shared_ptr<peer::IdentityManager> idmgr_;
    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;
    // secio conn deps go below
    std::shared_ptr<crypto::hmac::HmacProvider> hmac_provider_;
    //
    secio::ProposeMessage propose_message_;
    mutable common::ByteArray remote_peer_rand_;
    log::Logger log_ = log::createLogger("SecIO");
  };
}  // namespace libp2p::security

OUTCOME_HPP_DECLARE_ERROR(libp2p::security, Secio::Error);

#endif  // LIBP2P_SECIO_ADAPTOR_HPP
