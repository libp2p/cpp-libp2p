/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/multi/converters/conversion_error.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi::converters, ConversionError, e) {
  using libp2p::multi::converters::ConversionError;

  switch (e) {
    case ConversionError::INVALID_ADDRESS:
      return "Invalid address";
    case ConversionError::NO_SUCH_PROTOCOL:
      return "Protocol with given code does not exist";
    case ConversionError::ADDRESS_DOES_NOT_BEGIN_WITH_SLASH:
      return "An address should begin with a slash";
    case ConversionError::NOT_IMPLEMENTED:
      return "Conversion for this protocol is not implemented";
    default:
      return "Unknown error";
  }
}
