/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/host/basic_host/basic_host.hpp>

#include <boost/assert.hpp>
#include <libp2p/common/hexutil.hpp>
#include <libp2p/crypto/key_marshaller/key_marshaller_impl.hpp>

namespace libp2p::host {

  BasicHost::BasicHost(std::shared_ptr<peer::IdentityManager> idmgr,
                       std::unique_ptr<network::Network> network,
                       std::unique_ptr<peer::PeerRepository> repo,
                       std::shared_ptr<event::Bus> bus)
      : idmgr_(std::move(idmgr)),
        network_(std::move(network)),
        repo_(std::move(repo)),
        bus_(std::move(bus)) {
    BOOST_ASSERT(idmgr_ != nullptr);
    BOOST_ASSERT(network_ != nullptr);
    BOOST_ASSERT(repo_ != nullptr);
    BOOST_ASSERT(bus_ != nullptr);
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

    // TODO(xDimon): Needs to filter special interfaces (e.g. INADDR_ANY, etc.)

    std::set<multi::Multiaddress> unique_addresses;
    unique_addresses.insert(addresses.begin(), addresses.end());
    unique_addresses.insert(interfaces.begin(), interfaces.end());
    unique_addresses.insert(observed.begin(), observed.end());

    std::vector<multi::Multiaddress> unique_addr_list(unique_addresses.begin(),
                                                      unique_addresses.end());

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
