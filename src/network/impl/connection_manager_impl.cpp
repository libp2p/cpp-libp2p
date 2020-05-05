/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/network/impl/connection_manager_impl.hpp>

#include <algorithm>

namespace libp2p::network {

  std::vector<ConnectionManager::ConnectionSPtr>
  ConnectionManagerImpl::getConnectionsToPeer(const peer::PeerId &p) const {
    auto it = connections_.find(p);
    if (it == connections_.end()) {
      return {};
    }

    return it->second;
  }

  ConnectionManager::ConnectionSPtr
  ConnectionManagerImpl::getBestConnectionForPeer(const peer::PeerId &p) const {
    // TODO(warchant): maybe make pluggable strategies
    for (auto &conn : getConnectionsToPeer(p)) {
      if (!conn->isClosed()) {
        // for now, return first connection
        return conn;
      }
    }
    return nullptr;
  }

  ConnectionManager::Connectedness ConnectionManagerImpl::connectedness(
      const peer::PeerInfo &p) const {
    auto it = connections_.find(p.id);
    if (it != connections_.end()) {
      // if all connections are nullptr or closed
      if (it->second.empty() ||
          std::all_of(
            it->second.begin(), it->second.end(),
            [](auto &&conn) {
               return conn == nullptr || conn->isClosed();
            }
          )
      ) {
        return Connectedness::NOT_CONNECTED;
      }

      // valid connections have been found
      return Connectedness::CONNECTED;
    }
    // no valid connections found

    // if no connectios to this peer
    if (p.addresses.empty()) {
      return Connectedness::CAN_NOT_CONNECT;
    }

    // for each address, try to find transport to dial
    for (auto &&ma : p.addresses) {
      if (auto tr = transport_manager_->findBest(ma); tr != nullptr) {
        // we can dial to the peer
        return Connectedness::CAN_CONNECT;
      }
    }

    // we did not find available transports to dial
    return Connectedness::CAN_NOT_CONNECT;
  }

  void ConnectionManagerImpl::addConnectionToPeer(
      const peer::PeerId &p, ConnectionManager::ConnectionSPtr c) {
    auto it = connections_.find(p);
    if (it == connections_.end()) {
      connections_.insert({p, {c}});
    } else {
      connections_[p].push_back(c);
    }
    bus_->getChannel<event::OnNewConnectionChannel>().publish(c);
  }

  std::vector<ConnectionManager::ConnectionSPtr>
  ConnectionManagerImpl::getConnections() const {
    std::vector<ConnectionSPtr> out;
    out.reserve(connections_.size());

    for (auto &&entry : connections_) {
      out.insert(out.end(), entry.second.begin(), entry.second.end());
    }

    return out;
  }

  ConnectionManagerImpl::ConnectionManagerImpl(
      std::shared_ptr<libp2p::event::Bus> bus,
      std::shared_ptr<TransportManager> tmgr)
      : transport_manager_(std::move(tmgr)), bus_(std::move(bus)) {
    BOOST_ASSERT(transport_manager_ != nullptr);
  }

  void ConnectionManagerImpl::collectGarbage() {
    for (auto it = connections_.begin(); it != connections_.end();) {
      auto &vec = it->second;

      // remove all nullptr and closed connections
      vec.erase(std::remove_if(vec.begin(), vec.end(),
                               [](auto &&conn) {
                                 return conn == nullptr || conn->isClosed();
                               }),
                vec.end());

      // if peer has no connections, remove peer
      if (vec.empty()) {
        it = connections_.erase(it);
      } else {
        ++it;
      }
    }
  }

  void ConnectionManagerImpl::closeConnectionsToPeer(const peer::PeerId &p) {
    for (auto &&conn : getConnectionsToPeer(p)) {
      if (!conn->isClosed()) {
        // ignore errors
        (void)conn->close();
      }
    }

    connections_.erase(p);
  }

}  // namespace libp2p::network
