/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/network/impl/connection_manager_impl.hpp>

#include <algorithm>

namespace libp2p::network {

  namespace {
    auto log() {
      static auto logger = libp2p::log::createLogger("ConnectionManager");
      return logger.get();
    }
  }  // namespace

  std::vector<ConnectionManager::ConnectionSPtr>
  ConnectionManagerImpl::getConnectionsToPeer(const peer::PeerId &p) const {
    auto it = connections_.find(p);
    if (it == connections_.end()) {
      return {};
    }

    return std::vector<ConnectionManager::ConnectionSPtr>(it->second.begin(),
                                                          it->second.end());
  }

  ConnectionManager::ConnectionSPtr
  ConnectionManagerImpl::getBestConnectionForPeer(const peer::PeerId &p) const {
    // TODO(warchant): maybe make pluggable strategies

    auto it = connections_.find(p);
    if (it != connections_.end()) {
      for (const auto &conn : it->second) {
        if (!conn->isClosed()) {
          // for now, return first connection
          return conn;
        }
      }
    }
    return nullptr;
  }

  void ConnectionManagerImpl::addConnectionToPeer(
      const peer::PeerId &p, ConnectionManager::ConnectionSPtr c) {
    if (c == nullptr) {
      log()->error("inconsistency: not adding nullptr to active connections");
      return;
    }

    auto it = connections_.find(p);
    if (it == connections_.end()) {
      connections_.insert({p, {c}});
    } else {
      connections_[p].insert(c);
    }
    bus_->getChannel<event::network::OnNewConnectionChannel>().publish(c);
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
      std::shared_ptr<libp2p::event::Bus> bus)
      : bus_(std::move(bus)) {}

  void ConnectionManagerImpl::collectGarbage() {
    for (auto it = connections_.begin(); it != connections_.end();) {
      auto &cs = it->second;
      for (auto it2 = cs.begin(); it2 != cs.end();) {
        const auto &conn = *it2;
        if (conn->isClosed()) {
          it2 = cs.erase(it2);
        } else {
          ++it2;
        }
      }

      // if peer has no connections, remove peer
      if (cs.empty()) {
        it = connections_.erase(it);
      } else {
        ++it;
      }
    }
  }

  void ConnectionManagerImpl::closeConnectionsToPeer(const peer::PeerId &p) {
    auto it = connections_.find(p);
    if (it == connections_.end()) {
      return;
    }

    auto connections = std::move(it->second);
    connections_.erase(it);

    if (connections.empty()) {
      log()->error("inconsistency: iterator and no peers");
      return;
    }

    closing_connections_to_peer_ = p;

    for (const auto &conn : connections) {
      if (!conn->isClosed()) {
        // ignore errors
        (void)conn->close();
      }
    }

    closing_connections_to_peer_.reset();

    // until all reentrancy issues are resolved, we cannot be sure whether new
    // connections not appeared during close() calls, which may call their
    // external callbacks
    if (connections_.count(p) == 0) {
      bus_->getChannel<event::network::OnPeerDisconnectedChannel>().publish(p);
    }
  }

  void ConnectionManagerImpl::onConnectionClosed(
      const peer::PeerId &peer_id,
      const std::shared_ptr<connection::CapableConnection> &conn) {
    if (closing_connections_to_peer_.has_value()
        && closing_connections_to_peer_.value() == peer_id) {
      return;
    }
    auto it = connections_.find(peer_id);
    if (it == connections_.end()) {
      log()->error("inconsistency in onConnectionClosed, peer not found");
      return;
    }

    [[maybe_unused]] auto erased = it->second.erase(conn);
    if (erased == 0) {
      log()->error("inconsistency in onConnectionClosed, connection not found");
    }

    if (it->second.empty()) {
      connections_.erase(peer_id);
      bus_->getChannel<event::network::OnPeerDisconnectedChannel>().publish(
          peer_id);
    }
  }

}  // namespace libp2p::network
