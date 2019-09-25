/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MULTISELECT_ERROR_HPP
#define KAGOME_MULTISELECT_ERROR_HPP

#include <outcome/outcome-register.hpp>

namespace libp2p::protocol_muxer {
  enum class MultiselectError {
    PROTOCOLS_LIST_EMPTY = 1,
    NEGOTIATION_FAILED,
    INTERNAL_ERROR,
    PROTOCOL_VIOLATION
  };
}

OUTCOME_HPP_DECLARE_ERROR(libp2p::protocol_muxer, MultiselectError)

#endif  // KAGOME_MULTISELECT_ERROR_HPP
