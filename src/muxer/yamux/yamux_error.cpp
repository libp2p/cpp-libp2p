/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/yamux/yamux_error.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, YamuxError, e) {
  using E = libp2p::connection::YamuxError;
  switch (e) {
    case E::CONNECTION_STOPPED:
      return "Yamux: connection is stopped";
    case E::INTERNAL_ERROR:
      return "Yamux: internal error";
    case E::FORBIDDEN_CALL:
      return "Yamux: call is forbidden: use streams";
    case E::INVALID_ARGUMENT:
      return "Yamux: invalid argument";
    case E::TOO_MANY_STREAMS:
      return "Yamux: too many streams";
    case E::STREAM_IS_READING:
      return "Yamux: stream is reading";
    case E::STREAM_NOT_READABLE:
      return "Yamux: stream is not readable";
    case E::STREAM_NOT_WRITABLE:
      return "Yamux: stream is not writable";
    case E::STREAM_WRITE_BUFFER_OVERFLOW:
      return "Yamux: stream write buffer overflow: slow peer";
    case E::STREAM_CLOSED_BY_HOST:
      return "Yamux: stream closed by host";
    case E::STREAM_CLOSED_BY_PEER:
      return "Yamux: stream closed by peer";
    case E::STREAM_RESET_BY_HOST:
      return "Yamux: stream reset by host";
    case E::STREAM_RESET_BY_PEER:
      return "Yamux: stream reset by peer";
    case E::INVALID_WINDOW_SIZE:
      return "Yamux: invalid window size";
    case E::RECEIVE_WINDOW_OVERFLOW:
      return "Yamux: receive window overflow";
    case E::CONNECTION_CLOSED_BY_HOST:
      return "Yamux: connection closed by host";
    case E::CONNECTION_CLOSED_BY_PEER:
      return "Yamux: connection closed by peer";
    case E::PROTOCOL_ERROR:
      return "Yamux: protocol violation or garbage received from peer";
    default:
      break;
  }
  return "Yamux: unknown error";
}

