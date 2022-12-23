/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_RAW_CONNECTION_HPP
#define LIBP2P_RAW_CONNECTION_HPP

#include <libp2p/connection/layer_connection.hpp>

namespace libp2p::connection {

  /**
   * @bried Represents raw network connection, which is neither encrypted nor multiplexed.
   */
  struct RawConnection : public LayerConnection {
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

    ~RawConnection() override = default;
  };

}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, RawConnection::Error)

#endif  // LIBP2P_RAW_CONNECTION_HPP
