/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::peer {

  enum class PeerError { SUCCESS = 0, NOT_FOUND };

}

OUTCOME_HPP_DECLARE_ERROR(libp2p::peer, PeerError)
