/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PLAINTEXT_ADAPTOR_HPP
#define LIBP2P_PLAINTEXT_ADAPTOR_HPP

#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/log/logger.hpp>
#include <libp2p/peer/identity_manager.hpp>
#include <libp2p/security/plaintext/exchange_message_marshaller.hpp>
#include <libp2p/security/security_adaptor.hpp>

namespace libp2p::basic {
  class ProtobufMessageReadWriter;
}

namespace libp2p::security {
  /**
   * Implementation of security adaptor, which creates plaintext connection.
   *
   * Protocol is the following:
   * 1. Initiator immediately sends his public key to the other peer.
   * 2. Responder receives public key, saves it
   * 3. Responder answers with his public key
   * 4. Initiator calculates PeerId from responder's public key, and if it
   * differs from the one supplied in dial, yields an error
   */
  class Plaintext : public SecurityAdaptor,
                    public std::enable_shared_from_this<Plaintext> {
   public:
    enum class Error {
      EXCHANGE_SEND_ERROR = 1,
      EXCHANGE_RECEIVE_ERROR,
      INVALID_PEER_ID,
      EMPTY_PEER_ID
    };

    ~Plaintext() override = default;

    Plaintext(
        std::shared_ptr<plaintext::ExchangeMessageMarshaller> marshaller,
        std::shared_ptr<peer::IdentityManager> idmgr,
        std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller);

    peer::ProtocolName getProtocolId() const override;

    void secureInbound(std::shared_ptr<connection::LayerConnection> inbound,
                       SecConnCallbackFunc cb) override;

    void secureOutbound(std::shared_ptr<connection::LayerConnection> outbound,
                        const peer::PeerId &p, SecConnCallbackFunc cb) override;

   private:
    using MaybePeerId = boost::optional<peer::PeerId>;

    void sendExchangeMsg(
        const std::shared_ptr<connection::LayerConnection> &conn,
        const std::shared_ptr<basic::ProtobufMessageReadWriter> &rw,
        SecConnCallbackFunc cb) const;

    void receiveExchangeMsg(
        const std::shared_ptr<connection::LayerConnection> &conn,
        const std::shared_ptr<basic::ProtobufMessageReadWriter> &rw,
        const MaybePeerId &p, SecConnCallbackFunc cb) const;

    // the callback passed to an async read call in receiveExchangeMsg
    void readCallback(const std::shared_ptr<connection::LayerConnection> &conn,
                      const MaybePeerId &p, const SecConnCallbackFunc &cb,
                      const std::shared_ptr<std::vector<uint8_t>> &read_bytes,
                      outcome::result<size_t> read_call_res) const;

    /**
     * Close (\param conn) and report error in case of failure
     */
    void closeConnection(
        const std::shared_ptr<libp2p::connection::LayerConnection> &conn,
        const std::error_code &err) const;

    std::shared_ptr<plaintext::ExchangeMessageMarshaller> marshaller_;
    std::shared_ptr<peer::IdentityManager> idmgr_;
    std::shared_ptr<crypto::marshaller::KeyMarshaller> key_marshaller_;
    log::Logger log_ = log::createLogger("Plaintext");
  };
}  // namespace libp2p::security

OUTCOME_HPP_DECLARE_ERROR(libp2p::security, Plaintext::Error);

#endif  // LIBP2P_PLAINTEXT_ADAPTOR_HPP
