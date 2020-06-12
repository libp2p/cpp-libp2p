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
    auto interfaces = getAddressesInterfaces();
    auto observed = getObservedAddresses();

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
    network_->getListener().getRouter().setProtocolHandler(
        proto,
        [this, proto,
         handler](const std::shared_ptr<connection::Stream> &stream) {
          if (auto remote_peer_id_res = stream->remotePeerId();
              remote_peer_id_res) {
            // store stream in the map if it does not exist
            const auto &remote_peer_id = remote_peer_id_res.value();
            const auto &[it, inserted] =
                open_streams_.insert({remote_peer_id, {}});
            auto &proto_map = it->second;

            // insert new stream if no stream for given peer id and protocol
            // exists or it exists but expired or it exists but closed
            if (proto_map.count(proto) == 0
                or proto_map[proto].lock() == nullptr
                or proto_map[proto].lock()->isClosed()) {
              spdlog::debug(
                  "Inserted new stream during handling for peer id {}, "
                  "protocol {}",
                  remote_peer_id.toBase58(), proto);
              open_streams_[remote_peer_id][proto] = stream;
            }
          }
          handler(stream);
        });
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
                            const Host::StreamResultHandler &handler) {
    if (const auto &id_it = open_streams_.find(p.id);
        id_it != open_streams_.end()) {
      auto &proto_map = id_it->second;
      if (const auto &stream_it = proto_map.find(protocol);
          stream_it != proto_map.end()) {
        if (auto stream = stream_it->second.lock();
            stream and not stream->isClosed()) {
          // stream with given protocol for given peer id already exists
          return handler(stream);
        }
        // stream either expired or closed, no need to keep it anymore
        proto_map.erase(stream_it);
      }
    }
    network_->getDialer().newStream(
        p, protocol,
        [this, p, protocol, handler](
            outcome::result<std::shared_ptr<connection::Stream>> stream_res) {
          if (stream_res) {
            // if peer id is not known insert empty map by this peer id (if
            // another map does not exist yet)
            const auto &[it, inserted] = open_streams_.insert({p.id, {}});

            auto &proto_map = it->second;
            // insert new stream if no stream for given peer id and protocol
            // exists or it exists but expired or it exists but closed
            if (proto_map.count(protocol) == 0
                or proto_map[protocol].lock() == nullptr
                or proto_map[protocol].lock()->isClosed()) {
              spdlog::debug(
                  "Inserted new stream during new stream for peer id {}, "
                  "protocol {}",
                  p.id.toBase58(), protocol);
              open_streams_[p.id].insert({protocol, stream_res.value()});
            }
          }
          handler(stream_res);
        });
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

  void BasicHost::connect(const peer::PeerInfo &p) {
    network_->getDialer().dial(p, [](auto && /* ignored */) {});
  }
}  // namespace libp2p::host
