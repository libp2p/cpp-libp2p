/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_YAMUX_ERROR_HPP
#define LIBP2P_YAMUX_ERROR_HPP

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::connection {
  enum class YamuxError {
    CONNECTION_STOPPED = 1,
    INTERNAL_ERROR,
    FORBIDDEN_CALL,
    INVALID_ARGUMENT,
    TOO_MANY_STREAMS,
    STREAM_IS_READING,
    STREAM_NOT_READABLE,
    STREAM_NOT_WRITABLE,
    STREAM_WRITE_BUFFER_OVERFLOW,
    STREAM_CLOSED_BY_HOST,
    STREAM_CLOSED_BY_PEER,
    STREAM_RESET_BY_HOST,
    STREAM_RESET_BY_PEER,
    INVALID_WINDOW_SIZE,
    RECEIVE_WINDOW_OVERFLOW,
    CONNECTION_CLOSED_BY_HOST,
    CONNECTION_CLOSED_BY_PEER,
    PROTOCOL_ERROR,
  };

}  // namespace libp2p::connection

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, YamuxError)

#endif  // LIBP2P_YAMUX_ERROR_HPP
