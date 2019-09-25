/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE_ERROR_HPP
#define KAGOME_BASE_ERROR_HPP

#include <outcome/outcome.hpp>

namespace libp2p::multi::detail {

  enum class BaseError {
    INVALID_BASE58_INPUT = 1,
    INVALID_BASE64_INPUT,
    NON_UPPERCASE_INPUT,
    NON_LOWERCASE_INPUT
  };

}

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi::detail, BaseError);

#endif  // KAGOME_BASE_ERROR_HPP
