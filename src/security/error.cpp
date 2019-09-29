/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/error.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security::SecurityError, e) {
  using E = libp2p::security::SecurityError;
  switch (e) {
    case E::AUTHENTICATION_ERROR:
      return "authentication error";
  }

  return "unknown error";
}
