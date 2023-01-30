/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_PROTOCOL_HPP
#define LIBP2P_PROTOCOL_HPP

#include <string>

namespace libp2p::peer {

  using ProtocolName = std::string;

  // libp2p::peer::Protocol conflicts with libp2p::multi::Protocol
  using Protocol [[deprecated("Use 'libp2p::peer::ProtocolName' instead")]] =
      std::string;

}  // namespace libp2p::peer

#endif  // LIBP2P_PROTOCOL_HPP
