/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECURITY_ADAPTOR_HPP
#define LIBP2P_SECURITY_ADAPTOR_HPP

#include <memory>

#include <libp2p/basic/adaptor.hpp>
#include <libp2p/connection/raw_connection.hpp>
#include <libp2p/connection/secure_connection.hpp>
#include <libp2p/outcome/outcome.hpp>
#include <libp2p/peer/peer_id.hpp>

namespace libp2p::security {

  /**
   * @brief Adaptor, which is used as base class for all security modules (e.g.
   * SECIO, Noise, TLS...)
   */
  struct SecurityAdaptor : public basic::Adaptor {
    using SecConnCallbackFunc = std::function<void(
        outcome::result<std::shared_ptr<connection::SecureConnection>>)>;

    ~SecurityAdaptor() override = default;

    /**
     * @brief Secure the connection, either locally or by communicating with
     * opposing node via inbound connection (received in listener).
     * @param inbound connection
     * @param cb with secured connection or error
     */
    virtual void secureInbound(
        std::shared_ptr<connection::LayerConnection> inbound,
        SecConnCallbackFunc cb) = 0;

    /**
     * @brief Secure the connection, either locally or by communicating with
     * opposing node via outbound connection (we are initiator).
     * @param outbound connection
     * @param p remote peer id, we want to establish a secure conn
     * @param cb with secured connection or error
     */
    virtual void secureOutbound(
        std::shared_ptr<connection::LayerConnection> outbound,
        const peer::PeerId &p, SecConnCallbackFunc cb) = 0;
  };
}  // namespace libp2p::security

#endif  // LIBP2P_SECURITY_ADAPTOR_HPP
