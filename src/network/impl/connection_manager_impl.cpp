/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
    std::vector<ConnectionSPtr> out;
    out.reserve(it->second.size());
    for (const auto &conn : it->second) {
      if (not conn->isClosed()) {
        out.emplace_back(conn);
      }
    }
    return out;
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
    SL_INFO(log(), "=== addConnectionToPeer CALLED ===");
    SL_INFO(log(), "peer: {}", p.toBase58());
    SL_INFO(log(), "connection address: {}", (void*)c.get());
    SL_INFO(log(), "connection use_count: {}", c.use_count());
    
    if (c == nullptr) {
      log()->error("inconsistency: not adding nullptr to active connections");
      return;
    }

    auto it = connections_.find(p);
    if (it == connections_.end()) {
      SL_INFO(log(), "Creating new peer entry in connections_");
      connections_.insert({p, {c}});
    } else {
      SL_INFO(log(), "Adding to existing peer entry (current size: {})", it->second.size());
      connections_[p].insert(c);
    }
    
    SL_INFO(log(), "Connection successfully added. Total peers: {}", connections_.size());
    auto it_check = connections_.find(p);
    if (it_check != connections_.end()) {
      SL_INFO(log(), "Verification: peer {} now has {} connections", 
              p.toBase58(), it_check->second.size());
    }
    
    bus_->getChannel<event::network::OnNewConnectionChannel>().publish(c);
    SL_INFO(log(), "=== addConnectionToPeer FINISHED ===");
  }

  std::vector<ConnectionManager::ConnectionSPtr>
  ConnectionManagerImpl::getConnections() const {
    std::vector<ConnectionSPtr> out;
    out.reserve(connections_.size());

    for (auto &&entry : connections_) {
      for (const auto &conn : entry.second) {
        if (not conn->isClosed()) {
          out.emplace_back(conn);
        }
      }
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

  void ConnectionManagerImpl::onConnectionClosed(const peer::PeerId &peer,
                                                 CapableConnectionSPtr connection) {
    SL_INFO(log(), "=== onConnectionClosed CALLED ===");
    SL_INFO(log(), "peer: {}", peer.toBase58());
    SL_INFO(log(), "connection address: {}", (void*)connection.get());
    SL_INFO(log(), "connection use_count: {}", connection.use_count());
    
    if (closing_connections_to_peer_.has_value()
        && closing_connections_to_peer_.value() == peer) {
      return;
    }
    
    auto it = connections_.find(peer);
    if (it == connections_.end()) {
      SL_WARN(log(), "onConnectionClosed called for peer {} that was not in connection manager (connection may have been closed before being added)", peer.toBase58());
      return;
    }

    SL_INFO(log(), "Found peer in connections_, set size: {}", it->second.size());
    
    // Log all connections in set for comparison
    for (const auto& conn : it->second) {
      SL_INFO(log(), "Existing connection in set: address={}, use_count={}", 
              (void*)conn.get(), conn.use_count());
    }

    std::unique_lock lock(connection_mutex_);
    if (connection_is_closing_.count(connection) != 0) {
      SL_WARN(log(), "Connection {} is already being closed", (void*)connection.get());
      return;
    }
    connection_is_closing_.insert(connection);
    lock.unlock();

    auto erased = it->second.erase(connection);
    SL_INFO(log(), "Erased count: {}", erased);
    SL_INFO(log(), "Peer connection set size after erase: {}", it->second.size());
    
    if (erased == 0) {
      SL_WARN(log(), "Connection {} was not found in connections_ for peer {}", 
              (void*)connection.get(), peer.toBase58());
    } else {
      SL_INFO(log(), "Connection {} was successfully removed", (void*)connection.get());
    }

    if (it->second.empty()) {
      connections_.erase(peer);
      SL_INFO(log(), "Peer {} removed from connections_ (no more connections)", peer.toBase58());
      bus_->getChannel<event::network::OnPeerDisconnectedChannel>().publish(peer);
    }
    
    SL_INFO(log(), "=== onConnectionClosed FINISHED ===");
  }

}  // namespace libp2p::network
