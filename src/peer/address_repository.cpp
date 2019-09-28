/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/peer/address_repository.hpp>

namespace libp2p::peer {

  boost::signals2::connection AddressRepository::onAddressAdded(
      const std::function<AddressCallback> &cb) {
    return signal_added_.connect(cb);
  }

  boost::signals2::connection AddressRepository::onAddressRemoved(
      const std::function<AddressCallback> &cb) {
    return signal_removed_.connect(cb);
  }

}  // namespace libp2p::peer
