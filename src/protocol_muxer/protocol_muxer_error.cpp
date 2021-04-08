/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/protocol_muxer.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer, ProtocolMuxer::Error, e) {
  using Errors = libp2p::protocol_muxer::ProtocolMuxer::Error;
  switch (e) {
    case Errors::NEGOTIATION_FAILED:
      return "ProtocolMuxer: protocol negotiation failed";
    case Errors::INTERNAL_ERROR:
      return "ProtocolMuxer: internal error";
    case Errors::PROTOCOL_VIOLATION:
      return "ProtocolMuxer: peer error, incompatible multiselect protocol";
  }
  return "ProtocolMuxer: unknown";
}
