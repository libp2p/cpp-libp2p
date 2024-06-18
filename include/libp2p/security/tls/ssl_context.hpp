/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace boost::asio::ssl {
  class context;
}

namespace libp2p::peer {
  class IdentityManager;
}  // namespace libp2p::peer

namespace libp2p::crypto::marshaller {
  class KeyMarshaller;
}  // namespace libp2p::crypto::marshaller

namespace libp2p::security {
  /**
   * SSL context with libp2p TLS 1.3 certificate
   */
  struct SslContext {
    SslContext(const peer::IdentityManager &idmgr,
               const crypto::marshaller::KeyMarshaller &key_marshaller);

    std::shared_ptr<boost::asio::ssl::context> tls;
    std::shared_ptr<boost::asio::ssl::context> quic;
  };
}  // namespace libp2p::security
