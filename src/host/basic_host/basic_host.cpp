/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/host/basic_host/basic_host.hpp>

#include <boost/assert.hpp>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>

namespace libp2p::host {

  BasicHost::BasicHost(
      std::shared_ptr<peer::IdentityManager> idmgr,
      std::unique_ptr<network::Network> network,
      std::unique_ptr<peer::PeerRepository> repo,
      std::shared_ptr<event::Bus> bus,
      std::shared_ptr<network::TransportManager> transport_manager)
      : idmgr_(std::move(idmgr)),
        network_(std::move(network)),
        repo_(std::move(repo)),
        bus_(std::move(bus)),
        transport_manager_(std::move(transport_manager)) {
    BOOST_ASSERT(idmgr_ != nullptr);
    BOOST_ASSERT(network_ != nullptr);
    BOOST_ASSERT(repo_ != nullptr);
    BOOST_ASSERT(bus_ != nullptr);
    BOOST_ASSERT(transport_manager_ != nullptr);
  }

  std::string_view BasicHost::getLibp2pVersion() const {
    return "0.0.0";
  }

  std::string_view BasicHost::getLibp2pClientVersion() const {
    return "libp2p";
  }

  peer::PeerId BasicHost::getId() const {
    return idmgr_->getId();
  }

  peer::PeerInfo BasicHost::getPeerInfo() const {
    auto addresses = getAddresses();
    auto observed = getObservedAddresses();
    auto interfaces = getAddressesInterfaces();

    std::set<multi::Multiaddress> unique_addresses;
    unique_addresses.insert(std::make_move_iterator(addresses.begin()),
                            std::make_move_iterator(addresses.end()));
    unique_addresses.insert(std::make_move_iterator(interfaces.begin()),
                            std::make_move_iterator(interfaces.end()));
    unique_addresses.insert(std::make_move_iterator(observed.begin()),
                            std::make_move_iterator(observed.end()));

    // TODO(xDimon): Needs to filter special interfaces (e.g. INADDR_ANY, etc.)
    for (auto i = unique_addresses.begin(); i != unique_addresses.end();) {
      bool is_good_addr = true;
      for (auto &pv : i->getProtocolsWithValues()) {
        if (pv.first.code == multi::Protocol::Code::IP4) {
          if (pv.second == "0.0.0.0") {
            is_good_addr = false;
            break;
          }
        } else if (pv.first.code == multi::Protocol::Code::IP6) {
          if (pv.second == "::") {
            is_good_addr = false;
            break;
          }
        }
      }
      if (not is_good_addr) {
        i = unique_addresses.erase(i);
      } else {
        ++i;
      }
    }

    std::vector<multi::Multiaddress> unique_addr_list(
        std::make_move_iterator(unique_addresses.begin()),
        std::make_move_iterator(unique_addresses.end()));

    return {getId(), std::move(unique_addr_list)};
  }

  std::vector<multi::Multiaddress> BasicHost::getAddresses() const {
    return network_->getListener().getListenAddresses();
  }

  std::vector<multi::Multiaddress> BasicHost::getAddressesInterfaces() const {
    return network_->getListener().getListenAddressesInterfaces();
  }

  std::vector<multi::Multiaddress> BasicHost::getObservedAddresses() const {
    auto r = repo_->getAddressRepository().getAddresses(getId());
    if (r) {
      return r.value();
    }

    // we don't know our addresses
    return {};
  }

  Host::Connectedness BasicHost::connectedness(const peer::PeerInfo &p) const {
    auto conn = network_->getConnectionManager().getBestConnectionForPeer(p.id);
    if (conn != nullptr) {
      return Connectedness::CONNECTED;
    }

    // for each address, try to find transport to dial
    for (auto &&ma : p.addresses) {
      if (auto tr = transport_manager_->findBest(ma); tr != nullptr) {
        // we can dial to the peer
        return Connectedness::CAN_CONNECT;
      }
    }

    auto res = repo_->getAddressRepository().getAddresses(p.id);
    if (res.has_value()) {
      for (auto &&ma : res.value()) {
        if (auto tr = transport_manager_->findBest(ma); tr != nullptr) {
          // we can dial to the peer
          return Connectedness::CAN_CONNECT;
        }
      }
    }

    // we did not find available transports to dial
    return Connectedness::CAN_NOT_CONNECT;
  }

  void BasicHost::setProtocolHandler(
      const peer::Protocol &proto,
      const std::function<connection::Stream::Handler> &handler) {
    network_->getListener().getRouter().setProtocolHandler(proto, handler);
  }

  void BasicHost::setProtocolHandler(
      const peer::Protocol &proto,
      const std::function<connection::Stream::Handler> &handler,
      const std::function<bool(const peer::Protocol &)> &predicate) {
    network_->getListener().getRouter().setProtocolHandler(proto, handler,
                                                           predicate);
  }

  void BasicHost::newStream(const peer::PeerInfo &p,
                            const peer::Protocol &protocol,
                            const Host::StreamResultHandler &handler,
                            std::chrono::milliseconds timeout) {
    network_->getDialer().newStream(p, protocol, handler, timeout);
  }

  void BasicHost::newStream(const peer::PeerId &peer_id,
                            const peer::Protocol &protocol,
                            const StreamResultHandler &handler) {
    network_->getDialer().newStream(peer_id, protocol, handler);
  }

  outcome::result<void> BasicHost::listen(const multi::Multiaddress &ma) {
    return network_->getListener().listen(ma);
  }

  outcome::result<void> BasicHost::closeListener(
      const multi::Multiaddress &ma) {
    return network_->getListener().closeListener(ma);
  }

  outcome::result<void> BasicHost::removeListener(
      const multi::Multiaddress &ma) {
    return network_->getListener().removeListener(ma);
  }

  void BasicHost::start() {
    network_->getListener().start();
  }

  event::Handle BasicHost::setOnNewConnectionHandler(
      const NewConnectionHandler &h) const {
    return bus_->getChannel<network::event::OnNewConnectionChannel>().subscribe(
        [h{std::move(h)}](auto &&conn) {
          if (auto connection = conn.lock()) {
            auto remote_peer_res = connection->remotePeer();
            if (!remote_peer_res)
              return;

            auto remote_peer_addr_res = connection->remoteMultiaddr();
            if (!remote_peer_addr_res)
              return;

            if (h != nullptr)
              h(peer::PeerInfo{std::move(remote_peer_res.value()),
                               std::vector<multi::Multiaddress>{
                                   std::move(remote_peer_addr_res.value())}});
          }
        });
  }

  void BasicHost::stop() {
    network_->getListener().stop();
  }

  network::Network &BasicHost::getNetwork() {
    return *network_;
  }

  peer::PeerRepository &BasicHost::getPeerRepository() {
    return *repo_;
  }

  network::Router &BasicHost::getRouter() {
    return network_->getListener().getRouter();
  }

  event::Bus &BasicHost::getBus() {
    return *bus_;
  }

  void BasicHost::connect(const peer::PeerInfo &peer_info,
                          const ConnectionResultHandler &handler,
                          std::chrono::milliseconds timeout) {
    network_->getDialer().dial(peer_info, handler, timeout);
  }

  void BasicHost::disconnect(const peer::PeerId &peer_id) {
    network_->closeConnections(peer_id);
  }

}  // namespace libp2p::host
