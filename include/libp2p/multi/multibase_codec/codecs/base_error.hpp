/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::multi::detail {

  enum class BaseError {
    INVALID_BASE58_INPUT = 1,
    INVALID_BASE64_INPUT,
    INVALID_BASE32_INPUT,
    NON_UPPERCASE_INPUT,
    NON_LOWERCASE_INPUT
  };

}

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi::detail, BaseError);
