/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/muxer/mplex/mplex_error.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::connection, MplexError, e) {
  using E = libp2p::connection::MplexError;
  switch (e) {
    case E::BAD_FRAME_FORMAT:
      return "the other side has sent something, which is not a valid Mplex "
             "frame";
    case E::TOO_MANY_STREAMS:
      return "number of streams exceeds the maximum";
  }
  return "unknown MplexError";
}
