/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_MPLEX_ERROR_HPP
#define LIBP2P_MPLEX_ERROR_HPP

#include <outcome/outcome.hpp>

namespace libp2p::connection {
  enum class MplexError { BAD_FRAME_FORMAT = 1, TOO_MANY_STREAMS };
}

OUTCOME_HPP_DECLARE_ERROR(libp2p::connection, MplexError)

#endif  // LIBP2P_MPLEX_ERROR_HPP
