/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/security/noise/noise.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::security, Noise::Error, e) {
  using E = libp2p::security::Noise::Error;
  switch (e) {  // NOLINT
    case E::FAILURE:
      return "failure";
    default:
      return "Unknown error";
  }
}

namespace libp2p::security {
  peer::Protocol Noise::getProtocolId() const {
    return kProtocolId;
  }

  void Noise::secureInbound(std::shared_ptr<connection::RawConnection> inbound,
                            SecurityAdaptor::SecConnCallbackFunc cb) {
    log_->info("securing inbound connection");

  }


  void Noise::secureOutbound(
      std::shared_ptr<connection::RawConnection> outbound,
      const peer::PeerId &p, SecurityAdaptor::SecConnCallbackFunc cb) {
    log_->info("securing outbound connection");

  }
}  // namespace libp2p::security
