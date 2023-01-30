/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_CONNECTION_LAYERCONNECTION
#define LIBP2P_CONNECTION_LAYERCONNECTION

#include <libp2p/basic/readwritecloser.hpp>
#include <libp2p/multi/multiaddress.hpp>

namespace libp2p::connection {

  /**
   * @brief Class that represents a connection on some layer
   */
  struct LayerConnection : public basic::ReadWriteCloser {
    enum class Error {
      CONNECTION_INTERNAL_ERROR = 1,
      CONNECTION_INVALID_ARGUMENT,
      CONNECTION_PROTOCOL_ERROR,
      CONNECTION_NOT_ACTIVE,
      CONNECTION_TOO_MANY_STREAMS,
      CONNECTION_DIRECT_IO_FORBIDDEN,
      CONNECTION_CLOSED_BY_HOST,
      CONNECTION_CLOSED_BY_PEER,
    };

    ~LayerConnection() override = default;

    /// returns if this side is an initiator of this connection, or false if it
    /// was a server in that case
    virtual bool isInitiator() const noexcept = 0;

    /**
     * @brief Get local multiaddress for this connection.
     */
    virtual outcome::result<multi::Multiaddress> localMultiaddr() = 0;

    /**
     * @brief Get remote multiaddress for this connection.
     */
    virtual outcome::result<multi::Multiaddress> remoteMultiaddr() = 0;
  };

}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, LayerConnection::Error)

#endif  // LIBP2P_CONNECTION_LAYERCONNECTION
