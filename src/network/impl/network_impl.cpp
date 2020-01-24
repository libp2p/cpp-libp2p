/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/network/impl/network_impl.hpp>

namespace libp2p::network {

  void NetworkImpl::closeConnections(const peer::PeerId &p) {
    cmgr_->closeConnectionsToPeer(p);
  }

  Dialer &NetworkImpl::getDialer() {
    return *dialer_;
  }

  ListenerManager &NetworkImpl::getListener() {
    return *listener_;
  }

  ConnectionManager &NetworkImpl::getConnectionManager() {
    return *cmgr_;
  }

  NetworkImpl::NetworkImpl(std::shared_ptr<ListenerManager> listener,
                           std::unique_ptr<Dialer> dialer,
                           std::shared_ptr<ConnectionManager> cmgr)
      : listener_(std::move(listener)),
        dialer_(std::move(dialer)),
        cmgr_(std::move(cmgr)) {
    BOOST_ASSERT(listener_ != nullptr);
    BOOST_ASSERT(dialer_ != nullptr);
    BOOST_ASSERT(cmgr_ != nullptr);
  }
}  // namespace libp2p::network
