/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MULTI_CONVERTERS_CONVERSION_ERROR_HPP_
#define LIBP2P_MULTI_CONVERTERS_CONVERSION_ERROR_HPP_

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi::converters {

  /**
   * An error that might occur during conversion of
   * a multiaddr between byte format and string format
   */
  enum class ConversionError {
    ADDRESS_DOES_NOT_BEGIN_WITH_SLASH = 1,
    EMPTY_PROTOCOL,
    NO_SUCH_PROTOCOL,
    NOT_IMPLEMENTED,
    EMPTY_ADDRESS,
    INVALID_ADDRESS,
  };

}  // namespace libp2p::multi::converters

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi::converters, ConversionError)

#endif  // LIBP2P_MULTI_CONVERTERS_CONVERSION_ERROR_HPP_
