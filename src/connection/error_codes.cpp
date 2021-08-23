/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/connection/stream.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, RawConnection::Error, e) {
  using E = libp2p::connection::RawConnection::Error;
  switch (e) {
    case E::CONNECTION_INTERNAL_ERROR:
      return "Connection: internal error";
    case E::CONNECTION_INVALID_ARGUMENT:
      return "Connection: invalid argument";
    case E::CONNECTION_PROTOCOL_ERROR:
      return "Connection: protocol error due to remote peer";
    case E::CONNECTION_NOT_ACTIVE:
      return "Connection: not active or expired";
    case E::CONNECTION_TOO_MANY_STREAMS:
      return "Connection: too many open streams";
    case E::CONNECTION_DIRECT_IO_FORBIDDEN:
      return "Connection: direct read/write is forbidden, use streams";
    case E::CONNECTION_CLOSED_BY_HOST:
      return "Connection: closed by this host";
    case E::CONNECTION_CLOSED_BY_PEER:
      return "Connection: closed by remote peer";
    default:
      break;
  }
  return "Connection: unknown error";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, Stream::Error, e) {
  using E = libp2p::connection::Stream::Error;
  switch (e) {
    case E::STREAM_INTERNAL_ERROR:
      return "Stream: internal error";
    case E::STREAM_INVALID_ARGUMENT:
      return "Stream: invalid argument";
    case E::STREAM_PROTOCOL_ERROR:
      return "Stream: protocol error due to remote stream";
    case E::STREAM_IS_READING:
      return "Stream: stream reading";
    case E::STREAM_NOT_READABLE:
      return "Stream: stream is not readable";
    case E::STREAM_NOT_WRITABLE:
      return "Stream: stream is not writable";
    case E::STREAM_CLOSED_BY_HOST:
      return "Stream: closed by this host";
    case E::STREAM_CLOSED_BY_PEER:
      return "Stream: closed by remote peer";
    case E::STREAM_RESET_BY_HOST:
      return "Stream: reset by this host";
    case E::STREAM_RESET_BY_PEER:
      return "Stream: reset by remote peer";
    case E::STREAM_INVALID_WINDOW_SIZE:
      return "Stream: invalid window size";
    case E::STREAM_WRITE_OVERFLOW:
      return "Stream: write buffers overflow";
    case E::STREAM_RECEIVE_OVERFLOW:
      return "Stream: read window overflow";
    default:
      break;
  }
  return "Stream: unknown error";
}
