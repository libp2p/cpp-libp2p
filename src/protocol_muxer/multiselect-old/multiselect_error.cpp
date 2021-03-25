/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/multiselect_error.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::protocol_muxer, MultiselectError, e) {
  using Errors = libp2p::protocol_muxer::MultiselectError;
  switch (e) {
    case Errors::PROTOCOLS_LIST_EMPTY:
      return "no protocols were provided";
    case Errors::NEGOTIATION_FAILED:
      return "there are no protocols, supported by both sides of the "
             "connection";
    case Errors::INTERNAL_ERROR:
      return "internal error happened in this multiselect instance";
    case Errors::PROTOCOL_VIOLATION:
      return "other side has violated a protocol and sent an unexpected "
             "message";
  }
  return "unknown";
}
