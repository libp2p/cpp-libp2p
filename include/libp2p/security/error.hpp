/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::security {

  enum class SecurityError { AUTHENTICATION_ERROR = 1 };

}

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::SecurityError);
