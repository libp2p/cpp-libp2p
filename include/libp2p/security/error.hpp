/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECURITY_ERROR_HPP
#define LIBP2P_SECURITY_ERROR_HPP

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::security {

  enum class SecurityError { AUTHENTICATION_ERROR = 1};

}

OUTCOME_HPP_DECLARE_ERROR(libp2p::security::SecurityError);

#endif  // LIBP2P_SECURITY_ERROR_HPP
