/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/conversion_error.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi::converters, ConversionError, e) {
  using E = libp2p::multi::converters::ConversionError;

  switch (e) {
    case E::ADDRESS_DOES_NOT_BEGIN_WITH_SLASH:
      return "An address should begin with a slash";
    case E::EMPTY_PROTOCOL:
      return "Empty protocol (single slash or duplicated slashes)";
    case E::NO_SUCH_PROTOCOL:
      return "Protocol with given code does not exist";
    case E::NOT_IMPLEMENTED:
      return "Conversion for this protocol is not implemented";
    case E::EMPTY_ADDRESS:
    return "Empty address (missed address or duplicated slashes)";
    case E::INVALID_ADDRESS:
      return "Invalid address";
    default:
      return "Unknown error (libp2p::multi::converters::ConversionError)";
  }
}
