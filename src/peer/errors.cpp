/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/errors.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::peer, PeerError, e) {
  using libp2p::peer::PeerError;

  switch (e) {
    case PeerError::SUCCESS:
      return "success";
    case PeerError::NOT_FOUND:
      return "not found";
  }

  return "unknown";
}
