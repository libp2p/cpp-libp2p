/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PLAINTEXT_SESSION_HPP
#define KAGOME_PLAINTEXT_SESSION_HPP

#include "connection/raw_connection.hpp"
#include "crypto/key_marshaller.hpp"
#include "security/security_adaptor.hpp"

namespace libp2p::security {

  /**
   * @brief A little helper which reduces callback hell in Plaintext.
   */
  class PlaintextSession
      : public std::enable_shared_from_this<PlaintextSession> {
   public:
    using PubkeyFunc = std::function<void(crypto::PublicKey)>;
    using ThenFunc = std::function<void()>;

    PlaintextSession(
        std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller,
        std::shared_ptr<connection::RawConnection> conn,
        SecurityAdaptor::SecConnCallbackFunc handler);

    void recvKey(PubkeyFunc f);

    void sendKey(const crypto::PublicKey &publicKey, ThenFunc then);

   private:
    std::shared_ptr<crypto::marshaller::KeyMarshaller> marshaller_;
    std::shared_ptr<connection::RawConnection> conn_;
    SecurityAdaptor::SecConnCallbackFunc handler_;

    std::vector<uint8_t> sendbuf_;
    std::vector<uint8_t> recvbuf_;
  };

}  // namespace libp2p::security

#endif  // KAGOME_PLAINTEXT_SESSION_HPP
