/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_RAW_CONNECTION_HPP
#define LIBP2P_RAW_CONNECTION_HPP

#include <libp2p/basic/readwritecloser.hpp>
#include <libp2p/multi/multiaddress.hpp>

namespace libp2p::connection {

  /**
   * @bried Represents raw network connection, which is neither encrypted nor multiplexed.
   */
  struct RawConnection : public basic::ReadWriteCloser {
    ~RawConnection() override = default;

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

#endif  // LIBP2P_RAW_CONNECTION_HPP
