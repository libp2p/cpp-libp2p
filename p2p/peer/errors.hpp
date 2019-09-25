/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEER_ERRORS_HPP
#define KAGOME_PEER_ERRORS_HPP

#include <outcome/outcome.hpp>

namespace libp2p::peer {

  enum class PeerError { SUCCESS = 0, NOT_FOUND };

}

OUTCOME_HPP_DECLARE_ERROR(libp2p::peer, PeerError)

#endif  // KAGOME_PEER_ERRORS_HPP
