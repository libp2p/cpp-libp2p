/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_SECURITY_TLS_ERRORS_HPP
#define LIBP2P_SECURITY_TLS_ERRORS_HPP

#include <libp2p/outcome/outcome.hpp>

namespace libp2p::security {

  enum class TlsError : int {
    TLS_CTX_INIT_FAILED = 1,
    TLS_INCOMPATIBLE_TRANSPORT,
    TLS_NO_CERTIFICATE,
    TLS_INCOMPATIBLE_CERTIFICATE_EXTENSION,
    TLS_PEER_VERIFY_FAILED,
    TLS_UNEXPECTED_PEER_ID,
    TLS_REMOTE_PEER_NOT_AVAILABLE,
    TLS_REMOTE_PUBKEY_NOT_AVAILABLE,
  };
}  // namespace libp2p::security

OUTCOME_HPP_DECLARE_ERROR(libp2p::security, TlsError);

#endif  // LIBP2P_SECURITY_TLS_ERRORS_HPP
