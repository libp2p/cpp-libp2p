/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/enum_error_code.hpp>

namespace libp2p {
  enum class QuicError {
    HANDSHAKE_FAILED,
    CONN_CLOSED,
    STREAM_CLOSED,
    TOO_MANY_STREAMS,
    CANT_CREATE_CONNECTION,
    CANT_OPEN_STREAM,
  };
  Q_ENUM_ERROR_CODE(QuicError) {
    using E = decltype(e);
    switch (e) {
      case E::HANDSHAKE_FAILED:
        return "HANDSHAKE_FAILED";
      case E::CONN_CLOSED:
        return "CONN_CLOSED";
      case E::STREAM_CLOSED:
        return "STREAM_CLOSED";
      case E::TOO_MANY_STREAMS:
        return "TOO_MANY_STREAMS";
      case E::CANT_CREATE_CONNECTION:
        return "CANT_CREATE_CONNECTION";
      case E::CANT_OPEN_STREAM:
        return "CANT_OPEN_STREAM";
    }
    abort();
  }
}  // namespace libp2p
